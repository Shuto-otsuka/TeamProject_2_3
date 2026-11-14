#include <GraphicsEngine/Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusion.hlsli>
#include <GraphicsEngine/Raytracing/RaytracingHit.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

struct RaytracingAmbientOcclusionTraceBindings
{
	uint geometry_depth_;
	uint material_normal_roughness_;
	uint top_level_structure_;
	uint top_level_hits_;
	uint raytracing_ambient_occlusion_constants_;
	uint raytracing_ambient_occlusion_raw_;
	uint debug_view_rays_;
};
ConstantBuffer<RaytracingAmbientOcclusionTraceBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<float> raw = ResourceDescriptorHeap[bindings.raytracing_ambient_occlusion_raw_];
	Texture2D<float> depth_texture = ResourceDescriptorHeap[bindings.geometry_depth_];
	uint width;
	uint height;
	depth_texture.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	uint2 pixel = id.xy;
	float depth = depth_texture.Load(int3(pixel, 0));
	if (depth == 0.0)
	{
		raw[pixel] = 1.0;
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<RaytracingAmbientOcclusionConstantBuffer> constants = ResourceDescriptorHeap[bindings.raytracing_ambient_occlusion_constants_];
	float2 uv = (float2(pixel) + 0.5) / float2(width, height);
	float4 world = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), scene.inverse_view_projection_);
	float3 world_position = world.xyz / world.w;

	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_normal_roughness_];
	uint normal_roughness = surface_normal_roughness.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(float2(normal_roughness & 0xFFF, (normal_roughness >> 12) & 0xFFF) / 4095.0);
	float3 origin = world_position + normal * constants.normal_bias_;

	RaytracingHitTables tables;
	tables.top_level_ = bindings.top_level_structure_;
	tables.hits_ = bindings.top_level_hits_;
	RaytracingAccelerationStructure top_level = ResourceDescriptorHeap[tables.top_level_];

	uint random_state = SeedFromPixel(pixel, constants.frame_ + 7919);
	uint ray_count = max(constants.ray_count_, 1);
	float openness_sum = 0.0;
	for (uint ray_index = 0; ray_index < ray_count; ray_index++)
	{
		float3 direction = CosineSampleHemisphere(random_state, normal);
		RayDesc ray;
		ray.Origin = origin + direction * constants.normal_bias_;
		ray.Direction = direction;
		ray.TMin = 0.001;
		ray.TMax = constants.ray_length_;

		RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;
		query.TraceRayInline(top_level, RAY_FLAG_NONE, 0xFF, ray);
		while (query.Proceed())
		{
			if (query.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE && !RaytracingHitIsPassthrough(tables, query.CandidateInstanceID(), query.CandidatePrimitiveIndex(), query.CandidateTriangleBarycentrics(), query.CandidateObjectToWorld3x4(), query.CandidateWorldToObject3x4(), query.WorldRayOrigin() + query.WorldRayDirection() * query.CandidateTriangleRayT()))
			{
				query.CommitNonOpaqueTriangleHit();
			}
		}
		if (scene.view_mode_ == VIEW_MODE_RAY_COUNT)
		{
			RWTexture2D<uint> ray_count_texture = ResourceDescriptorHeap[bindings.debug_view_rays_];
			InterlockedAdd(ray_count_texture[pixel], 1);
		}
		openness_sum += query.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? saturate(query.CommittedRayT() / max(constants.ray_length_, 0.0001)) : 1.0;
	}
	raw[pixel] = openness_sum / float(ray_count);
}
