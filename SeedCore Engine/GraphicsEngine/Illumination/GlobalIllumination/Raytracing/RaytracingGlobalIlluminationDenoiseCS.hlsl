#include <GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIllumination.hlsli>
#include <GraphicsEngine/Shader/Denoiser.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

static const float GLOBAL_ILLUMINATION_TEMPORAL_ALPHA = 0.05;
static const float GLOBAL_ILLUMINATION_HISTORY_CLIP_GAMMA = 1.0;
static const float GLOBAL_ILLUMINATION_DEPTH_SHARPNESS = 48.0;
static const float GLOBAL_ILLUMINATION_NORMAL_POWER = 8.0;

struct RaytracingGlobalIlluminationTemporalBindings
{
	uint raytracing_global_illumination_raw_;
	uint raytracing_global_illumination_confidence_;
	uint geometry_depth_;
	uint material_surface_normal_roughness_;
	uint material_velocity_;
	uint raytracing_global_illumination_constants_;
	uint raytracing_global_illumination_accumulated_history_;
	uint raytracing_global_illumination_ping_;
	uint raytracing_global_illumination_history_length_history_;
	uint raytracing_global_illumination_history_length_;
	uint raytracing_global_illumination_validity_;
	uint raytracing_global_illumination_variance_;
};
ConstantBuffer<RaytracingGlobalIlluminationTemporalBindings> temporal_bindings : register(b0, space1);

struct RaytracingGlobalIlluminationATrousBindings
{
	uint raytracing_global_illumination_source_;
	uint geometry_depth_;
	uint material_surface_normal_roughness_;
	uint raytracing_global_illumination_destination_;
};
ConstantBuffer<RaytracingGlobalIlluminationATrousBindings> atrous_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Temporal(uint3 id : SV_DispatchThreadID)
{
	Texture2D<float> depth_texture = ResourceDescriptorHeap[temporal_bindings.geometry_depth_];
	uint width;
	uint height;
	depth_texture.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	int2 pixel = int2(id.xy);
	RWTexture2D<float4> ping = ResourceDescriptorHeap[temporal_bindings.raytracing_global_illumination_ping_];
	RWTexture2D<float> history_length_output = ResourceDescriptorHeap[temporal_bindings.raytracing_global_illumination_history_length_];
	float depth = depth_texture.Load(int3(pixel, 0));
	if (depth == 0.0)
	{
		ping[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		history_length_output[pixel] = 0.0;
		return;
	}

	ConstantBuffer<RaytracingGlobalIlluminationConstantBuffer> constants = ResourceDescriptorHeap[temporal_bindings.raytracing_global_illumination_constants_];
	Texture2D<float4> raw = ResourceDescriptorHeap[temporal_bindings.raytracing_global_illumination_raw_];
	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[temporal_bindings.material_surface_normal_roughness_];
	int2 screen_max = int2(width, height) - 1;
	uint center_packed = surface_normal_roughness.Load(int3(pixel, 0));
	float3 center_normal = OctNormalDecode(float2(center_packed & 0xFFF, (center_packed >> 12) & 0xFFF) / 4095.0);
	float2 depth_gradient = DenoiserDepthGradient(depth_texture, pixel, screen_max);

	float weight_sum = 0.0;
	float3 filtered_raw = float3(0.0, 0.0, 0.0);
	DenoiserMoments moments = DenoiserMomentsInit();
	[unroll]
	for (int offset_y = -2; offset_y <= 2; ++offset_y)
	{
		[unroll]
		for (int offset_x = -2; offset_x <= 2; ++offset_x)
		{
			int2 neighbor = clamp(pixel + int2(offset_x, offset_y), int2(0, 0), screen_max);
			float4 neighbor_raw = raw.Load(int3(neighbor, 0));
			uint neighbor_packed = surface_normal_roughness.Load(int3(neighbor, 0));
			float3 neighbor_normal = OctNormalDecode(float2(neighbor_packed & 0xFFF, (neighbor_packed >> 12) & 0xFFF) / 4095.0);
			float weight = DenoiserSpatialWeight(depth, depth_gradient, int2(offset_x, offset_y), depth_texture.Load(int3(neighbor, 0)), center_normal, neighbor_normal, GLOBAL_ILLUMINATION_DEPTH_SHARPNESS, GLOBAL_ILLUMINATION_NORMAL_POWER) * neighbor_raw.a;
			filtered_raw += neighbor_raw.rgb * weight;
			weight_sum += weight;
			if (abs(offset_x) <= 1 && abs(offset_y) <= 1)
			{
				DenoiserMomentsAccumulate(moments, neighbor_raw.rgb, weight);
			}
		}
	}
	filtered_raw = weight_sum > 0.0001 ? filtered_raw / weight_sum : raw.Load(int3(pixel, 0)).rgb;

	float3 result = filtered_raw;
	bool valid = false;
	float reprojection = 0.0;
	float history_length = 1.0;
	if (constants.history_valid_ != 0)
	{
		float3 clip_min = DenoiserVarianceClipMin(moments, filtered_raw, GLOBAL_ILLUMINATION_HISTORY_CLIP_GAMMA);
		float3 clip_max = DenoiserVarianceClipMax(moments, filtered_raw, GLOBAL_ILLUMINATION_HISTORY_CLIP_GAMMA);
		Texture2D<float> confidence = ResourceDescriptorHeap[temporal_bindings.raytracing_global_illumination_confidence_];
		Texture2D<float2> velocity = ResourceDescriptorHeap[temporal_bindings.material_velocity_];
		Texture2D<float4> history = ResourceDescriptorHeap[temporal_bindings.raytracing_global_illumination_accumulated_history_];
		float2 uv = (float2(pixel) + 0.5) / float2(width, height);
		float2 previous_uv = DenoiserPreviousUv(uv, velocity.Load(int3(pixel, 0)));
		result = DenoiserTemporalBlend(history, previous_uv, clip_min, clip_max, filtered_raw, lerp(GLOBAL_ILLUMINATION_TEMPORAL_ALPHA, 1.0, confidence.Load(int3(pixel, 0))));
		reprojection = length((previous_uv - uv) * float2(width, height));
		valid = all(previous_uv >= 0.0) && all(previous_uv <= 1.0) && history.SampleLevel(sampler_linear_clamp, previous_uv, 0).a > 0.5;
		if (valid)
		{
			Texture2D<float> history_length_history = ResourceDescriptorHeap[temporal_bindings.raytracing_global_illumination_history_length_history_];
			history_length = min(history_length_history.SampleLevel(sampler_linear_clamp, previous_uv, 0) + 1.0, 255.0);
		}
	}
	ping[pixel] = float4(result, 1.0);
	history_length_output[pixel] = history_length;

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	if (scene.view_mode_ >= VIEW_MODE_DENOISER_RAW && scene.view_mode_ <= VIEW_MODE_DENOISER_REPROJECTION)
	{
		float3 mean = moments.weight_sum_ > 0.0001 ? moments.moment1_sum_ / moments.weight_sum_ : filtered_raw;
		float3 mean_of_squares = moments.weight_sum_ > 0.0001 ? moments.moment2_sum_ / moments.weight_sum_ : filtered_raw * filtered_raw;
		RWTexture2D<float2> validity = ResourceDescriptorHeap[temporal_bindings.raytracing_global_illumination_validity_];
		RWTexture2D<float> variance = ResourceDescriptorHeap[temporal_bindings.raytracing_global_illumination_variance_];
		validity[pixel] = float2(valid ? 1.0 : 0.0, reprojection);
		variance[pixel] = DenoiserLuminance(max(mean_of_squares - mean * mean, 0.0));
	}
}

float4 RaytracingGlobalIlluminationATrous(Texture2D<float4> source, Texture2D<float> depth_texture, Texture2D<uint> surface_normal_roughness, int2 pixel, int2 screen_max, int step)
{
	float depth = depth_texture.Load(int3(pixel, 0));
	if (depth == 0.0)
	{
		return float4(0.0, 0.0, 0.0, 0.0);
	}

	uint center_packed = surface_normal_roughness.Load(int3(pixel, 0));
	float3 center_normal = OctNormalDecode(float2(center_packed & 0xFFF, (center_packed >> 12) & 0xFFF) / 4095.0);
	float2 depth_gradient = DenoiserDepthGradient(depth_texture, pixel, screen_max);

	float3 sum = float3(0.0, 0.0, 0.0);
	float weight_sum = 0.0;
	[unroll]
	for (int offset_y = -2; offset_y <= 2; ++offset_y)
	{
		[unroll]
		for (int offset_x = -2; offset_x <= 2; ++offset_x)
		{
			int2 offset = int2(offset_x, offset_y) * step;
			int2 neighbor = clamp(pixel + offset, int2(0, 0), screen_max);
			float4 neighbor_sample = source.Load(int3(neighbor, 0));
			uint neighbor_packed = surface_normal_roughness.Load(int3(neighbor, 0));
			float3 neighbor_normal = OctNormalDecode(float2(neighbor_packed & 0xFFF, (neighbor_packed >> 12) & 0xFFF) / 4095.0);
			float kernel_weight = DENOISER_ATROUS_KERNEL[offset_x + 2] * DENOISER_ATROUS_KERNEL[offset_y + 2];
			float edge_weight = DenoiserSpatialWeight(depth, depth_gradient, offset, depth_texture.Load(int3(neighbor, 0)), center_normal, neighbor_normal, GLOBAL_ILLUMINATION_DEPTH_SHARPNESS, GLOBAL_ILLUMINATION_NORMAL_POWER);
			float weight = kernel_weight * edge_weight * neighbor_sample.a;
			sum += neighbor_sample.rgb * weight;
			weight_sum += weight;
		}
	}
	return float4(weight_sum > 0.0001 ? sum / weight_sum : source.Load(int3(pixel, 0)).rgb, 1.0);
}

[numthreads(8, 8, 1)]
void ATrous(uint3 id : SV_DispatchThreadID)
{
	uint iteration = dispatch_bindings.constants_;
	Texture2D<float4> source = ResourceDescriptorHeap[atrous_bindings.raytracing_global_illumination_source_];
	uint width;
	uint height;
	source.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	Texture2D<float> depth_texture = ResourceDescriptorHeap[atrous_bindings.geometry_depth_];
	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[atrous_bindings.material_surface_normal_roughness_];
	RWTexture2D<float4> destination = ResourceDescriptorHeap[atrous_bindings.raytracing_global_illumination_destination_];
	destination[id.xy] = RaytracingGlobalIlluminationATrous(source, depth_texture, surface_normal_roughness, int2(id.xy), int2(width, height) - 1, int(1u << iteration));
}
