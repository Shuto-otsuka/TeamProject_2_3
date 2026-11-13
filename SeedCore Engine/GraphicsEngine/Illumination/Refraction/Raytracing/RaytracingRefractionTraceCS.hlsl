#include <GraphicsEngine/Raytracing/RaytracingHit.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

#define REFRACTION_MAX_BOUNCES 4

struct RaytracingRefractionConstantBuffer
{
	float ray_t_max_;
	float normal_bias_;
	float strength_;
	float raytracing_refraction_constant_buffer_padding_0_;
};

struct RaytracingRefractionTraceBindings
{
	uint geometry_depth_;
	uint material_surface_normal_roughness_;
	uint geometry_visibility_id_;
	uint geometry_instances_;
	uint top_level_structure_;
	uint top_level_hits_;
	uint sky_cubemap_;
	uint raytracing_refraction_constants_;
	uint raytracing_refraction_radiance_;
	uint debug_view_ray_count_;
};
ConstantBuffer<RaytracingRefractionTraceBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<float4> output = ResourceDescriptorHeap[bindings.raytracing_refraction_radiance_];
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
	Texture2D<uint2> visibility_id = ResourceDescriptorHeap[bindings.geometry_visibility_id_];
	if (depth == 0.0 || visibility_id.Load(int3(pixel, 0)).x == 0)
	{
		output[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	uint instance_index;
	uint meshlet_index;
	uint triangle_in_meshlet_index;
	UnpackVisibilityID(visibility_id.Load(int3(pixel, 0)), instance_index, meshlet_index, triangle_in_meshlet_index);
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[instance_index];
	if (instance.extension_.transmission_factor_ <= 0.0)
	{
		output[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<RaytracingRefractionConstantBuffer> constants = ResourceDescriptorHeap[bindings.raytracing_refraction_constants_];
	float2 uv = (float2(pixel) + 0.5) / float2(width, height);
	float4 world = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), scene.inverse_view_projection_);
	float3 world_position = world.xyz / world.w;

	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_surface_normal_roughness_];
	uint normal_roughness = surface_normal_roughness.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(float2(normal_roughness & 0xFFF, (normal_roughness >> 12) & 0xFFF) / 4095.0);
	float3 view_direction = normalize(world_position - scene.camera_position_.xyz);

	float3 ray_direction = refract(view_direction, normal, 1.0 / max(instance.texture_.ior_, 1.0001));
	if (dot(ray_direction, ray_direction) < 0.0001)
	{
		ray_direction = reflect(view_direction, normal);
	}
	ray_direction = normalize(ray_direction);

	RaytracingHitTables tables;
	tables.top_level_ = bindings.top_level_structure_;
	tables.hits_ = bindings.top_level_hits_;
	RaytracingAccelerationStructure top_level = ResourceDescriptorHeap[tables.top_level_];
	TextureCube<float4> sky_cubemap = ResourceDescriptorHeap[bindings.sky_cubemap_];

	float3 ray_origin = world_position + ray_direction * constants.normal_bias_;
	float3 throughput = float3(1.0, 1.0, 1.0);
	float3 radiance = float3(0.0, 0.0, 0.0);
	for (uint bounce = 0; bounce < REFRACTION_MAX_BOUNCES; bounce++)
	{
		RayDesc ray;
		ray.Origin = ray_origin;
		ray.Direction = ray_direction;
		ray.TMin = 0.001;
		ray.TMax = constants.ray_t_max_;

		RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;
		query.TraceRayInline(top_level, RAY_FLAG_NONE, 0xFF, ray);
		if (scene.view_mode_ == VIEW_MODE_RAY_COUNT)
		{
			RWTexture2D<uint> ray_count_texture = ResourceDescriptorHeap[bindings.debug_view_ray_count_];
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
			radiance = throughput * sky_cubemap.SampleLevel(sampler_linear_clamp, ray_direction, 0).rgb;
			break;
		}
		if (bounce + 1 >= REFRACTION_MAX_BOUNCES)
		{
			break;
		}

		float hit_t = query.CommittedRayT();
		float3 hit_position = ray_origin + ray_direction * hit_t;
		RaytracingHitSurface surface = RaytracingHitResolve(tables, query.CommittedInstanceID(), query.CommittedPrimitiveIndex(), query.CommittedTriangleBarycentrics(), query.CommittedObjectToWorld3x4(), query.CommittedWorldToObject3x4(), hit_position);
		if (surface.thickness_ > 0.0)
		{
			float3 optical_density = -log(max(surface.volume_attenuation_color_, 0.0001)) / max(surface.volume_attenuation_distance_, 0.0001);
			throughput *= exp(-optical_density * hit_t);
		}

		float3 next_direction = refract(ray_direction, surface.normal_, surface.ior_);
		if (dot(next_direction, next_direction) < 0.0001)
		{
			next_direction = reflect(ray_direction, surface.normal_);
		}
		ray_direction = normalize(next_direction);
		ray_origin = hit_position + ray_direction * constants.normal_bias_;
	}

	output[pixel] = float4(radiance, saturate(constants.strength_));
}
