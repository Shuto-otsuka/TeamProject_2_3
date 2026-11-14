#include <GraphicsEngine/Raytracing/RaytracingHit.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

struct RaytracingSubsurfaceScatteringConstantBuffer
{
	float scattering_distance_;
	float thickness_bias_;
	float ray_t_max_;
	float strength_;

	float3 subsurface_color_;
	float raytracing_subsurface_scattering_constant_buffer_padding_0_;
};

struct RaytracingSubsurfaceScatteringTraceBindings
{
	uint geometry_depth_;
	uint material_normal_roughness_;
	uint light_constants_;
	uint top_level_structure_;
	uint top_level_hits_;
	uint raytracing_subsurface_scattering_constants_;
	uint raytracing_subsurface_scattering_transmittance_;
	uint debug_view_rays_;
};
ConstantBuffer<RaytracingSubsurfaceScatteringTraceBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<float4> transmittance = ResourceDescriptorHeap[bindings.raytracing_subsurface_scattering_transmittance_];
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
	ConstantBuffer<LightConstantBuffer> light_constants = ResourceDescriptorHeap[bindings.light_constants_];
	if (depth == 0.0 || light_constants.sun_intensity_ <= 0.0)
	{
		transmittance[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_normal_roughness_];
	uint normal_roughness = surface_normal_roughness.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(float2(normal_roughness & 0xFFF, (normal_roughness >> 12) & 0xFFF) / 4095.0);
	float3 sun_direction = normalize(-light_constants.sun_direction_);
	if (dot(normal, sun_direction) >= 0.0)
	{
		transmittance[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<RaytracingSubsurfaceScatteringConstantBuffer> constants = ResourceDescriptorHeap[bindings.raytracing_subsurface_scattering_constants_];
	float2 uv = (float2(pixel) + 0.5) / float2(width, height);
	float4 world = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), scene.inverse_view_projection_);
	float3 world_position = world.xyz / world.w;

	RaytracingHitTables tables;
	tables.top_level_ = bindings.top_level_structure_;
	tables.hits_ = bindings.top_level_hits_;
	RaytracingAccelerationStructure top_level = ResourceDescriptorHeap[tables.top_level_];

	RayDesc ray;
	ray.Origin = world_position - normal * constants.thickness_bias_ + sun_direction * constants.thickness_bias_;
	ray.Direction = sun_direction;
	ray.TMin = 0.001;
	ray.TMax = constants.ray_t_max_;

	RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;
	query.TraceRayInline(top_level, RAY_FLAG_NONE, 0xFF, ray);
	if (scene.view_mode_ == VIEW_MODE_RAY_COUNT)
	{
		RWTexture2D<uint> ray_count_texture = ResourceDescriptorHeap[bindings.debug_view_rays_];
		InterlockedAdd(ray_count_texture[pixel], 1);
	}
	while (query.Proceed())
	{
		if (query.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE && !RaytracingHitIsPassthrough(tables, query.CandidateInstanceID(), query.CandidatePrimitiveIndex(), query.CandidateTriangleBarycentrics(), query.CandidateObjectToWorld3x4(), query.CandidateWorldToObject3x4(), query.WorldRayOrigin() + query.WorldRayDirection() * query.CandidateTriangleRayT()))
		{
			query.CommitNonOpaqueTriangleHit();
		}
	}
	if (query.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
	{
		transmittance[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	float thickness_transmittance = exp(-query.CommittedRayT() / max(constants.scattering_distance_, 0.0001));
	transmittance[pixel] = float4(constants.subsurface_color_ * constants.strength_ * thickness_transmittance, 1.0);
}
