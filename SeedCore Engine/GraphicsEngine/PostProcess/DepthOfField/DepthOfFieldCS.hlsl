#include <GraphicsEngine/Shader/Scene.hlsli>

struct DepthOfFieldConstantBuffer
{
	float focus_distance_;
	float focus_range_;
	float max_blur_radius_;
	float depth_of_field_padding_0_;
};

struct DepthOfFieldBlurBindings
{
	uint upscale_color_;
	uint geometry_depth_;
	uint depth_of_field_constants_;
	uint depth_of_field_color_;
};
ConstantBuffer<DepthOfFieldBlurBindings> bindings : register(b0, space1);

#define DOF_SAMPLE_COUNT 16
static const float GOLDEN_ANGLE = 2.39996323;

float3 DepthOfFieldReconstructWorldPosition(float2 uv, float depth, float4x4 inverse_view_projection)
{
	float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
	ndc.y = -ndc.y;
	float4 world_position = mul(ndc, inverse_view_projection);
	return world_position.xyz / world_position.w;
}

float DepthOfFieldLinearViewDepth(float2 uv, Texture2D<float> depth_texture, SceneConstantBuffer scene)
{
	float depth = depth_texture.SampleLevel(sampler_point_clamp, uv, 0);
	float3 world_position = DepthOfFieldReconstructWorldPosition(uv, depth, scene.inverse_view_projection_);
	return mul(float4(world_position, 1.0), scene.view_).z;
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> output = ResourceDescriptorHeap[bindings.depth_of_field_color_];
	ConstantBuffer<DepthOfFieldConstantBuffer> constants = ResourceDescriptorHeap[bindings.depth_of_field_constants_];

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

	float center_view_depth = DepthOfFieldLinearViewDepth(uv, depth_texture, scene);
	float center_coc = saturate(abs(center_view_depth - focus_distance) / max(focus_range, 0.0001));
	float blur_radius = center_coc * max_blur_radius;

	float3 accum = source.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	float weight_sum = 1.0;

	if (blur_radius > 0.0001)
	{
		for (uint i = 0; i < DOF_SAMPLE_COUNT; i++)
		{
			float radius = sqrt((float(i) + 0.5) / float(DOF_SAMPLE_COUNT));
			float angle = float(i) * GOLDEN_ANGLE;
			float2 sample_uv = uv + float2(cos(angle), sin(angle)) * radius * blur_radius;

			accum += source.SampleLevel(sampler_linear_clamp, sample_uv, 0).rgb;
			weight_sum += 1.0;
		}
	}

	output[dtid.xy] = float4(accum / weight_sum, 1.0);
}
