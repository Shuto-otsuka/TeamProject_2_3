#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflection.hlsli>
#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionReSTIR.hlsli>
#include <GraphicsEngine/Raytracing/RaytracingHit.hlsli>
#include <GraphicsEngine/Shader/Denoiser.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Sky/Sky.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

struct RaytracingReflectionTraceBindings
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
	uint raytracing_reflection_constants_;
	uint raytracing_reflection_reservoir_history_;
	uint raytracing_reflection_reservoir_;
	uint raytracing_reflection_reuse_;
	uint debug_view_ray_count_;
};
ConstantBuffer<RaytracingReflectionTraceBindings> bindings : register(b0, space1);

struct [raypayload] RaytracingReflectionPayload
{
	float3 radiance_
		: read(caller)
		: write(caller, closesthit, miss);

	float hit_distance_
		: read(caller)
		: write(caller, closesthit, miss);
};

[shader("raygeneration")]
void RaytracingReflectionRayGeneration()
{
	uint2 pixel = DispatchRaysIndex().xy;
	uint2 size = DispatchRaysDimensions().xy;
	RWStructuredBuffer<RaytracingReflectionReservoir> reservoir_output = ResourceDescriptorHeap[bindings.raytracing_reflection_reservoir_];
	uint reservoir_index = pixel.y * size.x + pixel.x;

	Texture2D<float> depth_texture = ResourceDescriptorHeap[bindings.geometry_depth_];
	float depth = depth_texture.Load(int3(pixel, 0));
	if (depth == 0.0)
	{
		reservoir_output[reservoir_index] = (RaytracingReflectionReservoir)0;
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<RaytracingReflectionConstantBuffer> constants = ResourceDescriptorHeap[bindings.raytracing_reflection_constants_];
	float2 uv = (float2(pixel) + 0.5) / float2(size);
	float4 world = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), scene.inverse_view_projection_);
	float3 world_position = world.xyz / world.w;

	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_surface_normal_roughness_];
	uint normal_roughness = surface_normal_roughness.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(float2(normal_roughness & 0xFFF, (normal_roughness >> 12) & 0xFFF) / 4095.0);
	float roughness = (normal_roughness >> 24) / 255.0;

	float3 view_direction = normalize(world_position - scene.camera_position_.xyz);
	float3 view = -view_direction;
	float3 mirror_direction = normalize(reflect(view_direction, normal));
	float normal_dot_view = saturate(dot(normal, view));
	uint random_state = SeedFromPixel(pixel, constants.frame_ + 2654435761u);
	RaytracingAccelerationStructure top_level = ResourceDescriptorHeap[bindings.top_level_structure_];

	uint ray_count = roughness < 0.01 ? 1 : max(constants.ray_count_, 1);
	RaytracingReflectionReservoir reservoir = (RaytracingReflectionReservoir)0;
	for (uint ray_index = 0; ray_index < ray_count; ray_index++)
	{
		float3 half_vector = SampleGgxVisibleNormal(Rand2(random_state), normal, view, roughness);
		float3 reflect_direction = normalize(2.0 * dot(view, half_vector) * half_vector - view);
		float sample_weight = SmithGgxG2OverG1(normal_dot_view, dot(normal, reflect_direction), roughness);

		RaytracingReflectionPayload payload;
		payload.radiance_ = float3(0.0, 0.0, 0.0);
		payload.hit_distance_ = 0.0;
		if (sample_weight > 0.0)
		{
			RayDesc ray;
			ray.Origin = world_position + normal * constants.normal_bias_;
			ray.Direction = reflect_direction;
			ray.TMin = 0.001;
			ray.TMax = constants.ray_t_max_;
			if (scene.view_mode_ == VIEW_MODE_RAY_COUNT)
			{
				RWTexture2D<uint> ray_count_texture = ResourceDescriptorHeap[bindings.debug_view_ray_count_];
				InterlockedAdd(ray_count_texture[pixel], 1);
			}
			TraceRay(top_level, RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES, 0xFF, 0, 0, 0, ray, payload);
			payload.radiance_ *= sample_weight;
		}
		else
		{
			reflect_direction = mirror_direction;
		}

		float hit_distance = (isnan(payload.hit_distance_) || isinf(payload.hit_distance_)) ? 0.0 : clamp(payload.hit_distance_, 0.0, 65504.0);
		RaytracingReflectionReservoir candidate = ReflectionReservoirFromSample(reflect_direction, DenoiserSanitizeRadiance(payload.radiance_), hit_distance);
		if (ray_index == 0)
		{
		    reservoir = candidate;
		}
		else
		{
		    reservoir = ReflectionReservoirCombine(reservoir, candidate, true, random_state);
		}
	}
	reservoir.receiver_normal_ = normal;
	reservoir.receiver_depth_ = abs(mul(float4(world_position, 1.0), scene.non_jitter_view_projection_).w);

	Texture2D<float2> velocity = ResourceDescriptorHeap[bindings.material_velocity_];
	float2 previous_uv = DenoiserPreviousUv(uv, velocity.Load(int3(pixel, 0)));
	uint reuse = constants.temporal_reuse_ != 0 ? DEBUG_REUSE_REJECTED : 0;
	if (constants.temporal_reuse_ != 0 && constants.history_valid_ != 0 && all(previous_uv >= 0.0) && all(previous_uv <= 1.0))
	{
		StructuredBuffer<RaytracingReflectionReservoir> reservoir_history = ResourceDescriptorHeap[bindings.raytracing_reflection_reservoir_history_];
		uint2 previous_pixel = min(uint2(previous_uv * float2(size)), size - 1);
		RaytracingReflectionReservoir temporal = reservoir_history[previous_pixel.y * size.x + previous_pixel.x];
		temporal.sample_m_ = min(temporal.sample_m_, REFLECTION_RESERVOIR_M_CAP * saturate(roughness / REFLECTION_RESERVOIR_M_CAP_ROUGHNESS));
		temporal.sample_age_ += 1.0;
		float expected_previous_depth = abs(mul(float4(world_position, 1.0), scene.previous_non_jitter_view_projection_).w);
		bool history_valid = temporal.receiver_depth_ > 0.0 && abs(temporal.receiver_depth_ - expected_previous_depth) <= REFLECTION_RESERVOIR_DEPTH_THRESHOLD * expected_previous_depth && dot(normal, temporal.receiver_normal_) >= REFLECTION_RESERVOIR_NORMAL_THRESHOLD && temporal.sample_age_ <= REFLECTION_RESERVOIR_MAX_AGE;
		reservoir = ReflectionReservoirCombine(reservoir, temporal, history_valid, random_state);
		reuse = history_valid ? DEBUG_REUSE_TEMPORAL : DEBUG_REUSE_REJECTED;
	}
	reservoir_output[reservoir_index] = reservoir;

	if (scene.view_mode_ >= VIEW_MODE_RESERVOIR_COUNT && scene.view_mode_ <= VIEW_MODE_RESERVOIR_REJECTION)
	{
		RWTexture2D<uint> reuse_texture = ResourceDescriptorHeap[bindings.raytracing_reflection_reuse_];
		reuse_texture[pixel] = reuse;
	}
}

[shader("miss")]
void RaytracingReflectionMiss(inout RaytracingReflectionPayload payload)
{
	TextureCube<float4> sky_cubemap = ResourceDescriptorHeap[bindings.sky_cubemap_];
	payload.radiance_ = sky_cubemap.SampleLevel(sampler_linear_clamp, WorldRayDirection(), 0).rgb;
	payload.hit_distance_ = 100000.0;
}

[shader("anyhit")]
void RaytracingReflectionAnyHit(inout RaytracingReflectionPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
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
void RaytracingReflectionClosestHit(inout RaytracingReflectionPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
	RaytracingHitTables tables;
	tables.top_level_ = bindings.top_level_structure_;
	tables.hits_ = bindings.top_level_hits_;
	float3 hit_position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	RaytracingHitSurface surface = RaytracingHitResolve(tables, InstanceID(), PrimitiveIndex(), attributes.barycentrics, ObjectToWorld3x4(), WorldToObject3x4(), hit_position);

	ConstantBuffer<RaytracingReflectionConstantBuffer> constants = ResourceDescriptorHeap[bindings.raytracing_reflection_constants_];
	ConstantBuffer<LightConstantBuffer> light_constants = ResourceDescriptorHeap[bindings.light_constants_];
	RaytracingHitLights lights;
	lights.list_ = bindings.light_list_;
	lights.constants_ = bindings.light_constants_;
	lights.cluster_grid_ = bindings.light_cluster_grid_;

	TextureCube<float4> diffuse_irradiance = ResourceDescriptorHeap[bindings.sky_diffuse_irradiance_];
	float3 lighting = diffuse_irradiance.SampleLevel(sampler_linear_clamp, surface.normal_, 0).rgb * light_constants.sky_intensity_;
	lighting += RaytracingHitLighting(tables, lights, hit_position, surface.normal_, constants.normal_bias_, constants.ray_t_max_);

	payload.radiance_ = surface.albedo_ * lighting;
	payload.hit_distance_ = RayTCurrent();
}
