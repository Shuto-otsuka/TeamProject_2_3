#include <GraphicsEngine/Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusion.hlsli>
#include <GraphicsEngine/Shader/Denoiser.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

static const float AMBIENT_OCCLUSION_TEMPORAL_ALPHA = 0.02;
static const float AMBIENT_OCCLUSION_HISTORY_CLIP_SIGMA = 2.0;

struct RaytracingAmbientOcclusionTemporalBindings
{
	uint raytracing_ambient_occlusion_raw_;
	uint geometry_depth_;
	uint material_normal_roughness_;
	uint material_velocity_;
	uint raytracing_ambient_occlusion_constants_;
	uint raytracing_ambient_occlusion_accumulated_history_;
	uint raytracing_ambient_occlusion_accumulated_;
	uint raytracing_ambient_occlusion_openness_;
	uint raytracing_ambient_occlusion_age_history_;
	uint raytracing_ambient_occlusion_age_;
	uint raytracing_ambient_occlusion_validity_;
	uint raytracing_ambient_occlusion_variance_;
};
ConstantBuffer<RaytracingAmbientOcclusionTemporalBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Temporal(uint3 id : SV_DispatchThreadID)
{
	Texture2D<float> depth_texture = ResourceDescriptorHeap[bindings.geometry_depth_];
	uint width;
	uint height;
	depth_texture.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	int2 pixel = int2(id.xy);
	RWTexture2D<float> accumulated = ResourceDescriptorHeap[bindings.raytracing_ambient_occlusion_accumulated_];
	RWTexture2D<float> openness = ResourceDescriptorHeap[bindings.raytracing_ambient_occlusion_openness_];
	RWTexture2D<float> age_output = ResourceDescriptorHeap[bindings.raytracing_ambient_occlusion_age_];
	float depth = depth_texture.Load(int3(pixel, 0));
	if (depth == 0.0)
	{
		accumulated[pixel] = 1.0;
		openness[pixel] = 1.0;
		age_output[pixel] = 0.0;
		return;
	}

	ConstantBuffer<RaytracingAmbientOcclusionConstantBuffer> constants = ResourceDescriptorHeap[bindings.raytracing_ambient_occlusion_constants_];
	Texture2D<float> raw = ResourceDescriptorHeap[bindings.raytracing_ambient_occlusion_raw_];
	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_normal_roughness_];
	int2 screen_max = int2(width, height) - 1;
	uint center_packed = surface_normal_roughness.Load(int3(pixel, 0));
	float3 center_normal = OctNormalDecode(float2(center_packed & 0xFFF, (center_packed >> 12) & 0xFFF) / 4095.0);
	float2 depth_gradient = DenoiserDepthGradient(depth_texture, pixel, screen_max);

	float weight_sum = 0.0;
	float weight_squared_sum = 0.0;
	float filtered_raw = 0.0;
	[unroll]
	for (int offset_y = -2; offset_y <= 2; ++offset_y)
	{
		[unroll]
		for (int offset_x = -2; offset_x <= 2; ++offset_x)
		{
			int2 neighbor = clamp(pixel + int2(offset_x, offset_y), int2(0, 0), screen_max);
			uint neighbor_packed = surface_normal_roughness.Load(int3(neighbor, 0));
			float3 neighbor_normal = OctNormalDecode(float2(neighbor_packed & 0xFFF, (neighbor_packed >> 12) & 0xFFF) / 4095.0);
			float weight = DenoiserSpatialWeight(depth, depth_gradient, int2(offset_x, offset_y), depth_texture.Load(int3(neighbor, 0)), center_normal, neighbor_normal, 48.0, 8.0);
			filtered_raw += raw.Load(int3(neighbor, 0)) * weight;
			weight_sum += weight;
			weight_squared_sum += weight * weight;
		}
	}
	filtered_raw /= max(weight_sum, 0.0001);

	float effective_count = max((weight_sum * weight_sum) / max(weight_squared_sum, 0.0001), 1.0);
	float adjusted_mean = (filtered_raw * effective_count + 2.0) / (effective_count + 4.0);
	float clip_radius = AMBIENT_OCCLUSION_HISTORY_CLIP_SIGMA * sqrt(adjusted_mean * (1.0 - adjusted_mean) / (effective_count + 4.0));

	float result = filtered_raw;
	float2 uv = (float2(pixel) + 0.5) / float2(width, height);
	Texture2D<float2> velocity = ResourceDescriptorHeap[bindings.material_velocity_];
	float2 previous_uv = DenoiserPreviousUv(uv, velocity.Load(int3(pixel, 0)));
	bool valid = constants.history_valid_ != 0 && all(previous_uv >= 0.0) && all(previous_uv <= 1.0);
	float age = 1.0;
	if (valid)
	{
		Texture2D<float> history = ResourceDescriptorHeap[bindings.raytracing_ambient_occlusion_accumulated_history_];
		float history_value = clamp(history.SampleLevel(sampler_linear_clamp, previous_uv, 0), filtered_raw - clip_radius, filtered_raw + clip_radius);
		result = lerp(history_value, filtered_raw, AMBIENT_OCCLUSION_TEMPORAL_ALPHA);
		Texture2D<float> age_history = ResourceDescriptorHeap[bindings.raytracing_ambient_occlusion_age_history_];
		age = min(age_history.SampleLevel(sampler_linear_clamp, previous_uv, 0) + 1.0, 255.0);
	}

	accumulated[pixel] = result;
	openness[pixel] = pow(saturate(result), max(constants.power_, 0.0001));
	age_output[pixel] = age;

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	if (scene.view_mode_ >= VIEW_MODE_DENOISER_RAW && scene.view_mode_ <= VIEW_MODE_DENOISER_REPROJECTION)
	{
		RWTexture2D<float2> validity = ResourceDescriptorHeap[bindings.raytracing_ambient_occlusion_validity_];
		RWTexture2D<float> variance = ResourceDescriptorHeap[bindings.raytracing_ambient_occlusion_variance_];
		validity[pixel] = float2(valid ? 1.0 : 0.0, length((previous_uv - uv) * float2(width, height)));
		variance[pixel] = adjusted_mean * (1.0 - adjusted_mean) / (effective_count + 4.0);
	}
}
