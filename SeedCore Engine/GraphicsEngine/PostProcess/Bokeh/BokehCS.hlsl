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

#define BOKEH_SAMPLE_COUNT 12
static const float BOKEH_TAU = 6.283185307;

float3 BokehReconstructWorldPosition(float2 uv, float depth, float4x4 inverse_view_projection)
{
	float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
	ndc.y = -ndc.y;
	float4 world_position = mul(ndc, inverse_view_projection);
	return world_position.xyz / world_position.w;
}

float BokehLinearViewDepth(float2 uv, Texture2D<float> depth_texture, SceneConstantBuffer scene)
{
	float depth = depth_texture.SampleLevel(sampler_point_clamp, uv, 0);
	float3 world_position = BokehReconstructWorldPosition(uv, depth, scene.inverse_view_projection_);
	return mul(float4(world_position, 1.0), scene.view_).z;
}

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
