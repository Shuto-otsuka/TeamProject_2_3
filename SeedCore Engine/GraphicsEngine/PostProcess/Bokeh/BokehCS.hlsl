#include <GraphicsEngine/Shader/Scene.hlsli>

struct BokehConstantBuffer
{
	float focus_distance_;
	float focus_range_;
	float max_blur_radius_;
	float highlight_threshold_;
	float highlight_intensity_;
	uint blade_count_;
	float2 bokeh_padding_0_;
};

struct BokehResolveBindings
{
	uint upscale_color_;
	uint geometry_depth_;
	uint depth_of_field_color_;
	uint bokeh_constants_;
	uint bokeh_color_;
};
ConstantBuffer<BokehResolveBindings> bindings : register(b0, space1);

/**
* [EN]
* Reference:
* - https://blog.voxagon.se/2018/05/04/bokeh-depth-of-field-in-single-pass.html
*
* (BokehPolygonRadiusScale below is plain polar geometry for a regular
* polygon, not a published rendering technique - no reference for it.)
*
* Bokeh highlight pass. Runs after DepthOfFieldCS.hlsl and read-modify-writes
* the same native-res UAV (no resources of its own) - only meaningful when
* both DepthOfFieldIndices.enabled_ and BokehIndices.enabled_ are set (see
* PostProcessRenderer::Dispatch).
*
* DepthOfFieldCS.hlsl's plain Vogel-disk average already smears bright
* out-of-focus points into soft circles, but averaging dilutes a single
* bright sample among many dim ones, so distinct "bokeh circle" highlights
* rarely show through. This pass fixes that the standard way: bright-pass
* (highlight_threshold_) the ORIGINAL (pre-blur) scene color at sample points
* traced along a blade_count_-sided polygon (the regular-polygon radius
* formula below turns the circular Vogel disk boundary into a hexagon/
* octagon/etc. outline, the classic aperture-blade look) scaled by this
* pixel's own circle of confusion, and ADDS the result on top of
* DepthOfFieldCS.hlsl's output instead of blending it in - so each bright
* point keeps its own visible shaped highlight rather than being averaged
* away.
*
* ---------------------------------------------------------------------
*
* [JP]
* ボケハイライトパス。DepthOfFieldCS.hlsl の後に走り、同じネイティブ解像度
* UAV を read-modify-write する(自前のリソースは持たない) —
* DepthOfFieldIndices.enabled_ と BokehIndices.enabled_ の両方が立っている
* 時だけ意味を持つ(PostProcessRenderer::Dispatch 参照)。
*
* DepthOfFieldCS.hlsl の単純な Vogel ディスク平均化でも、ピントの外れた
* 明るい点は既にソフトな円へにじむが、平均化は1つの明るいサンプルを多数の
* 暗いサンプルへ薄めてしまうため、はっきりした「ボケの円」ハイライトとして
* 見えることは稀。このパスは定番の方法で直す: 元の(ぼかす前の)シーン色を
* highlight_threshold_ でブライトパスし、blade_count_ 角形の輪郭
* (下の正多角形の半径式が円形の Vogel ディスク境界を六角形/八角形などの
* 輪郭へ変える — 絞り羽根の見た目)に沿ってこのピクセル自身の錯乱円で
* スケールしたサンプル点を取り、その結果を DepthOfFieldCS.hlsl の出力へ
* ブレンドではなく「加算」する — こうすることで、明るい点それぞれが平均化で
* 薄まらず、独立した形のあるハイライトとして残る。
*/

#define BOKEH_SAMPLE_COUNT 12
static const float BOKEH_TAU = 6.283185307;

/**
* Reconstructs world position from UV + device depth (mul takes the row
* vector on the left, this project's row_major convention).
*/
float3 BokehReconstructWorldPosition(float2 uv, float depth, float4x4 inverse_view_projection)
{
	float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
	ndc.y = -ndc.y;
	float4 world_position = mul(ndc, inverse_view_projection);
	return world_position.xyz / world_position.w;
}

/**
* Linear view-space depth at a UV, used to derive this pixel's circle of
* confusion the same way DepthOfFieldCS.hlsl does.
*/
float BokehLinearViewDepth(float2 uv, Texture2D<float> depth_texture, SceneConstantBuffer scene)
{
	float depth = depth_texture.SampleLevel(sampler_point_clamp, uv, 0);
	float3 world_position = BokehReconstructWorldPosition(uv, depth, scene.inverse_view_projection_);
	return mul(float4(world_position, 1.0), scene.view_).z;
}

/**
* Soft threshold weighting used before accumulating each polygon-outline
* sample below. Fades to 0 as luminance approaches threshold from above,
* rather than a hard cutoff.
*/
float3 BokehBrightPass(float3 color, float threshold)
{
	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	float weight = max(luminance - threshold, 0.0) / max(luminance, 0.0001);
	return color * weight;
}

float BokehPolygonRadiusScale(float angle, uint blade_count)
{
	float slice = BOKEH_TAU / float(blade_count);
	float local_angle = fmod(angle, slice) - slice * 0.5;
	return cos(slice * 0.5) / cos(local_angle);
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> output = ResourceDescriptorHeap[bindings.bokeh_color_];
	Texture2D<float4> depth_of_field = ResourceDescriptorHeap[bindings.depth_of_field_color_];
	ConstantBuffer<BokehConstantBuffer> constants = ResourceDescriptorHeap[bindings.bokeh_constants_];

	uint width, height;
	output.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	Texture2D<float4> source = ResourceDescriptorHeap[bindings.upscale_color_];
	Texture2D<float> depth_texture = ResourceDescriptorHeap[bindings.geometry_depth_];
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);

	float focus_distance = constants.focus_distance_;
	float focus_range = constants.focus_range_;
	float max_blur_radius = constants.max_blur_radius_;

	float center_view_depth = BokehLinearViewDepth(uv, depth_texture, scene);
	float center_coc = saturate(abs(center_view_depth - focus_distance) / max(focus_range, 0.0001));
	float blur_radius = center_coc * max_blur_radius;

	if (blur_radius <= 0.0001)
	{
		output[dtid.xy] = depth_of_field.Load(int3(dtid.xy, 0));
		return;
	}

	float threshold = constants.highlight_threshold_;
	float intensity = constants.highlight_intensity_;
	uint blade_count = max(constants.blade_count_, 3);

	float3 accum = float3(0.0, 0.0, 0.0);

	for (uint i = 0; i < BOKEH_SAMPLE_COUNT; i++)
	{
		float angle = (float(i) / float(BOKEH_SAMPLE_COUNT)) * BOKEH_TAU;
		float radius = blur_radius * BokehPolygonRadiusScale(angle, blade_count);
		float2 sample_uv = uv + float2(cos(angle), sin(angle)) * radius;

		accum += BokehBrightPass(source.SampleLevel(sampler_linear_clamp, sample_uv, 0).rgb, threshold);
	}

	float3 existing = depth_of_field.Load(int3(dtid.xy, 0)).rgb;
	output[dtid.xy] = float4(existing + (accum / float(BOKEH_SAMPLE_COUNT)) * intensity, 1.0);
}
