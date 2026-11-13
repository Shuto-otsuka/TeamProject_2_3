#include <GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIllumination.hlsli>
#include <GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationReSTIR.hlsli>
#include <GraphicsEngine/Raytracing/RaytracingHit.hlsli>
#include <GraphicsEngine/Shader/Denoiser.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

struct RaytracingGlobalIlluminationTraceBindings
{
	uint geometry_depth_;
	uint material_surface_normal_roughness_;
	uint material_velocity_;
	uint top_level_structure_;
	uint top_level_hits_;
	uint light_list_;
	uint light_constants_;
	uint light_cluster_grid_;
	uint sky_cubemap_;
	uint sky_diffuse_irradiance_;
	uint raytracing_global_illumination_constants_;
	uint raytracing_global_illumination_reservoir_history_;
	uint raytracing_global_illumination_reservoir_;
	uint raytracing_global_illumination_reuse_;
	uint debug_view_ray_count_;
};
ConstantBuffer<RaytracingGlobalIlluminationTraceBindings> bindings : register(b0, space1);

struct [raypayload] RaytracingGlobalIlluminationPayload
{
	float3 radiance_
		: read(caller)
		: write(caller, closesthit, miss);

	float hit_distance_
		: read(caller)
		: write(caller, closesthit, miss);

	float3 albedo_
		: read(caller)
		: write(caller, closesthit);

	float2 normal_
		: read(caller)
		: write(caller, closesthit);
};

[shader("raygeneration")]
void RaytracingGlobalIlluminationRayGeneration()
{
	uint2 pixel = DispatchRaysIndex().xy;
	uint2 size = DispatchRaysDimensions().xy;
	RWStructuredBuffer<RaytracingGlobalIlluminationReservoir> reservoir_output = ResourceDescriptorHeap[bindings.raytracing_global_illumination_reservoir_];
	uint reservoir_index = pixel.y * size.x + pixel.x;

	Texture2D<float> depth_texture = ResourceDescriptorHeap[bindings.geometry_depth_];
	float depth = depth_texture.Load(int3(pixel, 0));
	if (depth == 0.0)
	{
		reservoir_output[reservoir_index] = (RaytracingGlobalIlluminationReservoir)0;
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<RaytracingGlobalIlluminationConstantBuffer> constants = ResourceDescriptorHeap[bindings.raytracing_global_illumination_constants_];
	ConstantBuffer<LightConstantBuffer> light_constants = ResourceDescriptorHeap[bindings.light_constants_];
	float2 uv = (float2(pixel) + 0.5) / float2(size);
	float4 world = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), scene.inverse_view_projection_);
	float3 world_position = world.xyz / world.w;

	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_surface_normal_roughness_];
	uint normal_roughness = surface_normal_roughness.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(float2(normal_roughness & 0xFFF, (normal_roughness >> 12) & 0xFFF) / 4095.0);

	uint random_state = SeedFromPixel(pixel, constants.frame_ + 15486071u);
	RaytracingAccelerationStructure top_level = ResourceDescriptorHeap[bindings.top_level_structure_];
	TextureCube<float4> diffuse_irradiance = ResourceDescriptorHeap[bindings.sky_diffuse_irradiance_];

	uint ray_count = max(constants.ray_count_, 1);
	uint bounce_count = max(constants.bounce_count_, 1);
	RaytracingGlobalIlluminationReservoir reservoir = (RaytracingGlobalIlluminationReservoir)0;
	for (uint ray_index = 0; ray_index < ray_count; ray_index++)
	{
		float3 ray_direction = CosineSampleHemisphere(random_state, normal);
		float3 bounce_origin = world_position + normal * constants.normal_bias_;
		float3 bounce_direction = ray_direction;
		float3 throughput = float3(1.0, 1.0, 1.0);
		float3 path_radiance = float3(0.0, 0.0, 0.0);
		float first_hit_distance = 0.0;
		for (uint bounce = 0; bounce < bounce_count; bounce++)
		{
			RayDesc ray;
			ray.Origin = bounce_origin;
			ray.Direction = bounce_direction;
			ray.TMin = 0.001;
			ray.TMax = constants.ray_t_max_;

			RaytracingGlobalIlluminationPayload payload;
			payload.radiance_ = float3(0.0, 0.0, 0.0);
			payload.hit_distance_ = 0.0;
			payload.albedo_ = float3(0.0, 0.0, 0.0);
			payload.normal_ = float2(0.0, 0.0);
			if (scene.view_mode_ == VIEW_MODE_RAY_COUNT)
			{
				RWTexture2D<uint> ray_count_texture = ResourceDescriptorHeap[bindings.debug_view_ray_count_];
				InterlockedAdd(ray_count_texture[pixel], 1);
			}
			TraceRay(top_level, RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES, 0xFF, 0, 0, 0, ray, payload);
			path_radiance += throughput * payload.radiance_;
			if (bounce == 0)
			{
				first_hit_distance = payload.hit_distance_;
			}
			if (payload.hit_distance_ >= 100000.0)
			{
				break;
			}

			float3 hit_normal = OctNormalDecode(payload.normal_);
			throughput *= payload.albedo_;
			if (bounce + 1 >= bounce_count)
			{
				path_radiance += throughput * diffuse_irradiance.SampleLevel(sampler_linear_clamp, hit_normal, 0).rgb * light_constants.sky_intensity_;
				break;
			}
			bounce_origin = bounce_origin + bounce_direction * payload.hit_distance_ + hit_normal * constants.normal_bias_;
			bounce_direction = CosineSampleHemisphere(random_state, hit_normal);
		}

		RaytracingGlobalIlluminationReservoir candidate = GlobalIlluminationReservoirFromSample(world_position + ray_direction * first_hit_distance, normal, DenoiserSanitizeRadiance(path_radiance));
		if (ray_index == 0)
		{
			reservoir = candidate;
		}
		else
		{
			reservoir = GlobalIlluminationReservoirCombine(reservoir, candidate, true, random_state);
		}
	}
	reservoir.receiver_normal_ = normal;
	reservoir.receiver_depth_ = abs(mul(float4(world_position, 1.0), scene.non_jitter_view_projection_).w);

	Texture2D<float2> velocity = ResourceDescriptorHeap[bindings.material_velocity_];
	float2 previous_uv = DenoiserPreviousUv(uv, velocity.Load(int3(pixel, 0)));
	uint reuse = constants.temporal_reuse_ != 0 ? DEBUG_REUSE_REJECTED : 0;
	if (constants.temporal_reuse_ != 0 && constants.history_valid_ != 0 && all(previous_uv >= 0.0) && all(previous_uv <= 1.0))
	{
		StructuredBuffer<RaytracingGlobalIlluminationReservoir> reservoir_history = ResourceDescriptorHeap[bindings.raytracing_global_illumination_reservoir_history_];
		uint2 previous_pixel = min(uint2(previous_uv * float2(size)), size - 1);
		RaytracingGlobalIlluminationReservoir temporal = reservoir_history[previous_pixel.y * size.x + previous_pixel.x];
		temporal.sample_m_ = min(temporal.sample_m_, GI_RESERVOIR_M_CAP);
		temporal.sample_age_ += 1.0;
		float expected_previous_depth = abs(mul(float4(world_position, 1.0), scene.previous_non_jitter_view_projection_).w);
		bool history_valid = temporal.receiver_depth_ > 0.0 && abs(temporal.receiver_depth_ - expected_previous_depth) <= GI_RESERVOIR_DEPTH_THRESHOLD * expected_previous_depth && dot(normal, temporal.receiver_normal_) >= GI_RESERVOIR_NORMAL_THRESHOLD && temporal.sample_age_ <= GI_RESERVOIR_MAX_AGE;
		reservoir = GlobalIlluminationReservoirCombine(reservoir, temporal, history_valid, random_state);
		reuse = history_valid ? DEBUG_REUSE_TEMPORAL : DEBUG_REUSE_REJECTED;
	}
	reservoir_output[reservoir_index] = reservoir;

	if (scene.view_mode_ >= VIEW_MODE_RESERVOIR_COUNT && scene.view_mode_ <= VIEW_MODE_RESERVOIR_REJECTION)
	{
		RWTexture2D<uint> reuse_texture = ResourceDescriptorHeap[bindings.raytracing_global_illumination_reuse_];
		reuse_texture[pixel] = reuse;
	}
}

[shader("miss")]
void RaytracingGlobalIlluminationMiss(inout RaytracingGlobalIlluminationPayload payload)
{
	TextureCube<float4> sky_cubemap = ResourceDescriptorHeap[bindings.sky_cubemap_];
	payload.radiance_ = sky_cubemap.SampleLevel(sampler_linear_clamp, WorldRayDirection(), 0).rgb;
	payload.hit_distance_ = 100000.0;
}

[shader("anyhit")]
void RaytracingGlobalIlluminationAnyHit(inout RaytracingGlobalIlluminationPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
	RaytracingHitTables tables;
	tables.top_level_ = bindings.top_level_structure_;
	tables.hits_ = bindings.top_level_hits_;
	if (RaytracingHitIsPassthrough(tables, InstanceID(), PrimitiveIndex(), attributes.barycentrics, ObjectToWorld3x4(), WorldToObject3x4(), WorldRayOrigin() + WorldRayDirection() * RayTCurrent()))
	{
		IgnoreHit();
	}
}

[shader("closesthit")]
void RaytracingGlobalIlluminationClosestHit(inout RaytracingGlobalIlluminationPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
	RaytracingHitTables tables;
	tables.top_level_ = bindings.top_level_structure_;
	tables.hits_ = bindings.top_level_hits_;
	float3 hit_position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	RaytracingHitSurface surface = RaytracingHitResolve(tables, InstanceID(), PrimitiveIndex(), attributes.barycentrics, ObjectToWorld3x4(), WorldToObject3x4(), hit_position);
	float3 normal = dot(surface.normal_, WorldRayDirection()) > 0.0 ? -surface.normal_ : surface.normal_;

	ConstantBuffer<RaytracingGlobalIlluminationConstantBuffer> constants = ResourceDescriptorHeap[bindings.raytracing_global_illumination_constants_];
	RaytracingHitLights lights;
	lights.list_ = bindings.light_list_;
	lights.constants_ = bindings.light_constants_;
	lights.cluster_grid_ = bindings.light_cluster_grid_;
	float3 lighting = RaytracingHitLighting(tables, lights, hit_position, normal, constants.normal_bias_, constants.ray_t_max_);

	payload.radiance_ = surface.albedo_ * lighting;
	payload.hit_distance_ = RayTCurrent();
	payload.albedo_ = surface.albedo_;
	payload.normal_ = OctNormalEncode(normal);
}
