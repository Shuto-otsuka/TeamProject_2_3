#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflection.hlsli>
#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionReSTIR.hlsli>
#include <GraphicsEngine/Shader/Denoiser.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

struct RaytracingReflectionSpatialBindings
{
	uint geometry_depth_;
	uint material_normal_roughness_;
	uint raytracing_reflection_reservoir_;
	uint raytracing_reflection_constants_;
	uint raytracing_reflection_raw_;
	uint raytracing_reflection_confidence_;
	uint raytracing_reflection_reuse_;
};
ConstantBuffer<RaytracingReflectionSpatialBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
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
	RWTexture2D<float4> raw = ResourceDescriptorHeap[bindings.raytracing_reflection_raw_];
	RWTexture2D<float> confidence = ResourceDescriptorHeap[bindings.raytracing_reflection_confidence_];
	float depth = depth_texture.Load(int3(pixel, 0));
	if (depth == 0.0)
	{
		raw[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		confidence[pixel] = 0.0;
		return;
	}

	ConstantBuffer<RaytracingReflectionConstantBuffer> constants = ResourceDescriptorHeap[bindings.raytracing_reflection_constants_];
	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_normal_roughness_];
	StructuredBuffer<RaytracingReflectionReservoir> reservoirs = ResourceDescriptorHeap[bindings.raytracing_reflection_reservoir_];

	uint packed = surface_normal_roughness.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(float2(packed & 0xFFF, (packed >> 12) & 0xFFF) / 4095.0);
	float roughness = (packed >> 24) / 255.0;
	RaytracingReflectionReservoir reservoir = reservoirs[pixel.y * width + pixel.x];

	uint random_state = SeedFromPixel(uint2(pixel), constants.frame_ + 3266489917u);
	int2 screen_max = int2(width, height) - 1;
	float2 depth_gradient = DenoiserDepthGradient(depth_texture, pixel, screen_max);
	float roughness_tolerance = lerp(0.02, 0.3, roughness);
	uint spatial_count = 0;
	[unroll]
	for (uint sample_index = 0; sample_index < 8; sample_index++)
	{
		float angle = Rand(random_state) * 6.28318530718;
		float radius = sqrt(Rand(random_state)) * 16.0;
		int2 neighbor_offset = int2(round(float2(cos(angle), sin(angle)) * radius));
		int2 neighbor_pixel = clamp(pixel + neighbor_offset, int2(0, 0), screen_max);
		float neighbor_depth = depth_texture.Load(int3(neighbor_pixel, 0));
		uint neighbor_packed = surface_normal_roughness.Load(int3(neighbor_pixel, 0));
		float3 neighbor_normal = OctNormalDecode(float2(neighbor_packed & 0xFFF, (neighbor_packed >> 12) & 0xFFF) / 4095.0);
		float neighbor_roughness = (neighbor_packed >> 24) / 255.0;
		float spatial_weight = DenoiserSpatialWeight(depth, depth_gradient, neighbor_offset, neighbor_depth, normal, neighbor_normal, 48.0, 8.0);
		bool neighbor_valid = neighbor_depth > 0.0 && spatial_weight > 0.1 && abs(neighbor_roughness - roughness) <= roughness_tolerance;
		reservoir = ReflectionReservoirCombine(reservoir, reservoirs[neighbor_pixel.y * width + neighbor_pixel.x], neighbor_valid, random_state);
		spatial_count += neighbor_valid ? 1 : 0;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	if (scene.view_mode_ >= VIEW_MODE_RESERVOIR_COUNT && scene.view_mode_ <= VIEW_MODE_RESERVOIR_REJECTION)
	{
		RWTexture2D<uint> reuse = ResourceDescriptorHeap[bindings.raytracing_reflection_reuse_];
		reuse[pixel] = (reuse[pixel] & 0xFF) | (spatial_count << DEBUG_REUSE_SPATIAL_SHIFT);
	}

	raw[pixel] = float4(reservoir.sample_radiance_ * reservoir.sample_w_, reservoir.sample_hit_distance_);
	confidence[pixel] = saturate(reservoir.sample_m_ / REFLECTION_RESERVOIR_M_CAP);
}
