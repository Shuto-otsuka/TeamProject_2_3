#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Culling/Culling.hlsli>
#include <GraphicsEngine/Light/LightCluster.h>
#include <GraphicsEngine/Raytracing/RaytracingHit.hlsli>
#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionReSTIR.hlsli>
#include <GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationReSTIR.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

struct DebugViewClearBindings
{
	uint debug_view_ray_count_;
	uint geometry_debug_;
};
ConstantBuffer<DebugViewClearBindings> clear_bindings : register(b0, space1);

struct DebugViewResolveBindings
{
	uint geometry_depth_;
	uint geometry_visibility_id_;
	uint geometry_instances_;
	uint geometry_debug_;
	uint material_surface_base_color_;
	uint material_surface_normal_roughness_;
	uint material_velocity_;
	uint material_surface_radiance_;
	uint material_mip_;
	uint light_cluster_grid_;
	uint top_level_structure_;
	uint top_level_hits_;
	uint debug_view_ray_count_;
	uint raytracing_reflection_raw_;
	uint raytracing_reflection_radiance_;
	uint raytracing_reflection_reservoir_;
	uint raytracing_reflection_reuse_;
	uint raytracing_reflection_moments_;
	uint raytracing_reflection_history_length_;
	uint raytracing_reflection_validity_;
	uint raytracing_global_illumination_raw_;
	uint raytracing_global_illumination_radiance_;
	uint raytracing_global_illumination_reservoir_;
	uint raytracing_global_illumination_reuse_;
	uint raytracing_global_illumination_variance_;
	uint raytracing_global_illumination_history_length_;
	uint raytracing_global_illumination_validity_;
	uint raytracing_shadow_raw_;
	uint raytracing_shadow_visibility_;
	uint raytracing_shadow_moments_;
	uint raytracing_shadow_history_length_;
	uint raytracing_shadow_validity_;
	uint raytracing_ambient_occlusion_raw_;
	uint raytracing_ambient_occlusion_openness_;
	uint raytracing_ambient_occlusion_variance_;
	uint raytracing_ambient_occlusion_history_length_;
	uint raytracing_ambient_occlusion_validity_;
	uint radiance_color_;
	uint upscale_color_;
	uint kawase_bloom_radiance_;
	uint auto_exposure_exposure_;
	uint hud_display_color_;
};
ConstantBuffer<DebugViewResolveBindings> resolve_bindings : register(b0, space1);

struct DebugViewBoundsBindings
{
	uint geometry_instances_;
	uint culling_draw_lists_;
	uint culling_draw_arguments_;
	uint hud_display_color_;
};
ConstantBuffer<DebugViewBoundsBindings> bounds_bindings : register(b0, space1);

/**
* [EN]
* Zeroes the per-pixel counters the debug producers add into before they
* run: DebugView.RayCount (every ray-traced effect adds one per traced ray)
* and Geometry.Debug (the meshlet overdraw pass adds one per fragment).
*/
[numthreads(8, 8, 1)]
void Clear(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<uint> ray_count = ResourceDescriptorHeap[clear_bindings.debug_view_ray_count_];
	RWTexture2D<uint> debug = ResourceDescriptorHeap[clear_bindings.geometry_debug_];
	uint width;
	uint height;
	debug.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}
	ray_count[id.xy] = 0;
	debug[id.xy] = 0;
}

/**
* [EN]
* Replaces DisplayColor with a debug image, one thread per display pixel; the
* dispatch constant is the RenderViewMode. Render-resolution buffers are read
* at the pixel under the display pixel. Scalar modes produce a value in [0,1]
* that is shown on a blue-cyan-green-yellow-red heat ramp, id modes produce an
* integer that is hashed into a stable color, and the rest write a color
* directly. Buffer Overview splits the screen into a 3x3 grid of the buffer
* modes. ReSTIR and denoiser modes read the effect SceneConstantBuffer's
* debug_source_ selects. Surface modes leave the background black.
*/
[numthreads(8, 8, 1)]
void Resolve(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<float4> display_color = ResourceDescriptorHeap[resolve_bindings.hud_display_color_];
	uint display_width;
	uint display_height;
	display_color.GetDimensions(display_width, display_height);
	if (id.x >= display_width || id.y >= display_height)
	{
		return;
	}

	Texture2D<float> depth_texture = ResourceDescriptorHeap[resolve_bindings.geometry_depth_];
	uint render_width;
	uint render_height;
	depth_texture.GetDimensions(render_width, render_height);
	float2 uv = (float2(id.xy) + 0.5) / float2(display_width, display_height);
	uint mode = dispatch_bindings.constants_;
	if (mode == VIEW_MODE_BUFFER_OVERVIEW)
	{
		uint2 cell = min(uint2(uv * 3.0), uint2(2, 2));
		uv = frac(uv * 3.0);
		const uint overview_modes[9] =
		{
			VIEW_MODE_BUFFER_BASE_COLOR, VIEW_MODE_BUFFER_NORMAL, VIEW_MODE_BUFFER_ROUGHNESS,
			VIEW_MODE_BUFFER_METALLIC, VIEW_MODE_BUFFER_EMISSIVE, VIEW_MODE_BUFFER_DEPTH,
			VIEW_MODE_BUFFER_VELOCITY, VIEW_MODE_BUFFER_POSITION, VIEW_MODE_BUFFER_MATERIAL
		};
		mode = overview_modes[cell.y * 3 + cell.x];
	}

	uint2 pixel = min(uint2(uv * float2(render_width, render_height)), uint2(render_width - 1, render_height - 1));
	float depth = depth_texture.Load(int3(pixel, 0));
	SceneConstantBuffer scene = GetSceneConstantBuffer();
	float4 world = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), scene.inverse_view_projection_);
	float3 world_position = world.xyz / world.w;
	uint source = scene.debug_source_;

	Texture2D<uint2> visibility_id = ResourceDescriptorHeap[resolve_bindings.geometry_visibility_id_];
	uint2 packed = visibility_id.Load(int3(pixel, 0));
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[resolve_bindings.geometry_instances_];
	ModelStructuredBuffer instance = (ModelStructuredBuffer)0;
	if (packed.x != 0)
	{
		instance = instances[packed.x - 1];
	}
	bool surface = depth > 0.0 && packed.x != 0;

	float3 color = float3(0.2, 0.2, 0.2);
	float heat = -1.0;
	uint hashed = 0;
	bool hashing = false;

	if (mode == VIEW_MODE_BUFFER_DEPTH && depth > 0.0)
	{
		float4 view_position = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), scene.inverse_projection_);
		color = 1.0 - saturate(view_position.z / view_position.w / 50.0);
	}
	else if (mode == VIEW_MODE_BUFFER_VELOCITY && depth > 0.0)
	{
		Texture2D<float2> velocity = ResourceDescriptorHeap[resolve_bindings.material_velocity_];
		color = float3(velocity.Load(int3(pixel, 0)) * 0.5 + 0.5, 0.0);
	}
	else if (mode == VIEW_MODE_POST_COLOR || mode == VIEW_MODE_POST_LUMINANCE || mode == VIEW_MODE_UPSCALE_OUTPUT)
	{
		Texture2D<float4> upscale_color = ResourceDescriptorHeap[resolve_bindings.upscale_color_];
		float3 hdr = upscale_color.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
		if (mode == VIEW_MODE_POST_LUMINANCE)
		{
			heat = saturate((log2(max(dot(hdr, float3(0.2126, 0.7152, 0.0722)), 1e-6)) + 10.0) / 20.0);
		}
		else
		{
			color = hdr / (1.0 + hdr);
		}
	}
	else if (mode == VIEW_MODE_UPSCALE_INPUT)
	{
		Texture2D<float4> radiance_color = ResourceDescriptorHeap[resolve_bindings.radiance_color_];
		float3 hdr = radiance_color.Load(int3(pixel, 0)).rgb;
		color = hdr / (1.0 + hdr);
	}
	else if (mode == VIEW_MODE_POST_BLOOM)
	{
		Texture2D<float4> bloom = ResourceDescriptorHeap[resolve_bindings.kawase_bloom_radiance_];
		float3 hdr = bloom.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
		color = hdr / (1.0 + hdr);
	}
	else if (mode == VIEW_MODE_POST_EXPOSURE)
	{
		StructuredBuffer<float> exposure = ResourceDescriptorHeap[resolve_bindings.auto_exposure_exposure_];
		Texture2D<float4> upscale_color = ResourceDescriptorHeap[resolve_bindings.upscale_color_];
		float exposure_value = exposure[0];
		if (uv.y < 0.03)
		{
			heat = saturate((log2(max(exposure_value, 1e-6)) + 10.0) / 20.0);
		}
		else
		{
			color = saturate(upscale_color.SampleLevel(sampler_linear_clamp, uv, 0).rgb * exposure_value);
		}
	}
	else if (mode == VIEW_MODE_POST_TONEMAP)
	{
		if (uv.x < 0.5)
		{
			Texture2D<float4> upscale_color = ResourceDescriptorHeap[resolve_bindings.upscale_color_];
			color = saturate(upscale_color.SampleLevel(sampler_linear_clamp, uv, 0).rgb);
		}
		else
		{
			color = display_color[id.xy].rgb;
		}
		if (abs(uv.x - 0.5) * display_width < 1.0)
		{
			color = float3(1.0, 1.0, 1.0);
		}
	}
	else if (mode == VIEW_MODE_RAY_DISTANCE || mode == VIEW_MODE_RAY_HIT || mode == VIEW_MODE_RAY_INSTANCE || mode == VIEW_MODE_RAY_GEOMETRY)
	{
		float4 target = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.5, 1.0), scene.inverse_view_projection_);
		RayDesc ray;
		ray.Origin = scene.camera_position_.xyz;
		ray.Direction = normalize(target.xyz / target.w - scene.camera_position_.xyz);
		ray.TMin = 0.001;
		ray.TMax = 100000.0;

		RaytracingHitTables tables;
		tables.top_level_ = resolve_bindings.top_level_structure_;
		tables.hits_ = resolve_bindings.top_level_hits_;
		RaytracingAccelerationStructure top_level = ResourceDescriptorHeap[tables.top_level_];
		RayQuery<RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;
		query.TraceRayInline(top_level, RAY_FLAG_NONE, 0xFF, ray);
		while (query.Proceed())
		{
			if (query.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE && !RaytracingHitIsPassthrough(tables, query.CandidateInstanceID(), query.CandidatePrimitiveIndex(), query.CandidateTriangleBarycentrics(), query.CandidateObjectToWorld3x4(), query.CandidateWorldToObject3x4(), query.WorldRayOrigin() + query.WorldRayDirection() * query.CandidateTriangleRayT()))
			{
				query.CommitNonOpaqueTriangleHit();
			}
		}
		bool hit = query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
		if (mode == VIEW_MODE_RAY_HIT)
		{
			color = hit ? float3(0.2, 0.85, 0.3) : float3(0.85, 0.2, 0.2);
		}
		else if (hit && mode == VIEW_MODE_RAY_DISTANCE)
		{
			heat = saturate(log2(1.0 + query.CommittedRayT()) / log2(1001.0));
		}
		else if (hit)
		{
			hashing = true;
			hashed = mode == VIEW_MODE_RAY_INSTANCE ? query.CommittedInstanceIndex() + 1 : (query.CommittedInstanceID() + 1) * 131u + query.CommittedGeometryIndex();
		}
	}
	else if (mode == VIEW_MODE_RAY_COUNT && depth > 0.0)
	{
		Texture2D<uint> ray_count = ResourceDescriptorHeap[resolve_bindings.debug_view_ray_count_];
		heat = saturate(ray_count.Load(int3(pixel, 0)) / 16.0);
	}
	else if (depth > 0.0)
	{
		Texture2D<float4> surface_base_color = ResourceDescriptorHeap[resolve_bindings.material_surface_base_color_];
		Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[resolve_bindings.material_surface_normal_roughness_];
		Texture2D<float4> surface_radiance = ResourceDescriptorHeap[resolve_bindings.material_surface_radiance_];
		float4 base_color = surface_base_color.Load(int3(pixel, 0));
		uint normal_roughness = surface_normal_roughness.Load(int3(pixel, 0));
		float3 normal = OctNormalDecode(float2(normal_roughness & 0xFFF, (normal_roughness >> 12) & 0xFFF) / 4095.0);
		uint reservoir_index = pixel.y * render_width + pixel.x;
		const float3 lod_colors[8] =
		{
			float3(1.0, 1.0, 1.0), float3(0.2, 0.9, 0.2), float3(0.2, 0.5, 1.0), float3(1.0, 0.9, 0.2),
			float3(1.0, 0.5, 0.1), float3(0.9, 0.2, 0.9), float3(0.2, 0.9, 0.9), float3(0.9, 0.2, 0.2)
		};

		if (mode == VIEW_MODE_UNLIT)
		{
			color = base_color.rgb + surface_radiance.Load(int3(pixel, 0)).rgb;
		}
		else if (mode == VIEW_MODE_BUFFER_BASE_COLOR)
		{
			color = base_color.rgb;
		}
		else if (mode == VIEW_MODE_BUFFER_METALLIC)
		{
			color = base_color.aaa;
		}
		else if (mode == VIEW_MODE_BUFFER_NORMAL)
		{
			color = normal * 0.5 + 0.5;
		}
		else if (mode == VIEW_MODE_BUFFER_ROUGHNESS)
		{
			color = (float)(normal_roughness >> 24) / 255.0;
		}
		else if (mode == VIEW_MODE_BUFFER_EMISSIVE)
		{
			color = surface_radiance.Load(int3(pixel, 0)).rgb;
		}
		else if (mode == VIEW_MODE_BUFFER_POSITION)
		{
			color = frac(world_position * 0.1);
		}
		else if (mode == VIEW_MODE_BUFFER_MATERIAL && surface)
		{
			hashing = true;
			hashed = instance.texture_.base_color_texture_index_ * 31u + instance.texture_.normal_texture_index_ * 17u + instance.shading_.shading_model_ * 7u + asuint(instance.texture_.base_color_.r) + asuint(instance.texture_.base_color_.g) * 3u + asuint(instance.texture_.base_color_.b) * 5u;
		}
		else if (mode == VIEW_MODE_MESHLET_ID && surface)
		{
			hashing = true;
			hashed = packed.x * 747796405u + (packed.y >> 7) * 2891336453u;
		}
		else if (mode == VIEW_MODE_MESHLET_TRIANGLE && surface)
		{
			hashing = true;
			hashed = packed.x * 747796405u + packed.y * 2891336453u;
		}
		else if (mode == VIEW_MODE_MESHLET_CLUSTER && surface)
		{
			hashing = true;
			hashed = instance.geometry_.meshlet_buffer_index_ + 1;
		}
		else if (mode == VIEW_MODE_MESHLET_INSTANCE && surface)
		{
			hashing = true;
			hashed = packed.x;
		}
		else if (mode == VIEW_MODE_MESHLET_LEVEL && surface)
		{
			color = lod_colors[min(instance.streaming_.lod_level_, 7u)];
		}
		else if (mode == VIEW_MODE_LOD_COLORATION && surface)
		{
			color = lod_colors[min(instance.streaming_.lod_level_, 7u)] * (0.3 + 0.7 * dot(base_color.rgb, float3(0.2126, 0.7152, 0.0722)));
		}
		else if (mode == VIEW_MODE_MESHLET_ERROR && surface)
		{
			heat = saturate(instance.streaming_.lod_error_ / 4.0);
		}
		else if (mode == VIEW_MODE_MESHLET_CULLING && surface)
		{
			Texture2D<uint> debug = ResourceDescriptorHeap[resolve_bindings.geometry_debug_];
			color = (debug.Load(int3(pixel, 0)) & DEBUG_GEOMETRY_LATE) != 0 ? float3(1.0, 0.35, 0.2) : float3(0.2, 0.85, 0.35);
		}
		else if (mode == VIEW_MODE_MESHLET_OVERDRAW)
		{
			Texture2D<uint> debug = ResourceDescriptorHeap[resolve_bindings.geometry_debug_];
			heat = saturate(debug.Load(int3(pixel, 0)) / 8.0);
		}
		else if (mode == VIEW_MODE_LIGHT_COMPLEXITY || mode == VIEW_MODE_LIGHT_CLUSTER)
		{
			float view_depth = mul(float4(world_position, 1.0), scene.view_).z;
			uint2 tile_count = (uint2(render_width, render_height) + LIGHT_CLUSTER_TILE_SIZE - 1) / LIGHT_CLUSTER_TILE_SIZE;
			uint2 tile = min(pixel / LIGHT_CLUSTER_TILE_SIZE, tile_count - 1);
			uint slice = 0;
			if (view_depth >= scene.far_plane_)
			{
				slice = LIGHT_CLUSTER_DEPTH_SLICES - 1;
			}
			else if (view_depth > scene.near_plane_)
			{
				float log_near = log(scene.near_plane_);
				slice = min(uint((log(view_depth) - log_near) / (log(scene.far_plane_) - log_near) * LIGHT_CLUSTER_DEPTH_SLICES), LIGHT_CLUSTER_DEPTH_SLICES - 1);
			}
			uint cluster = (slice * tile_count.y + tile.y) * tile_count.x + tile.x;
			if (mode == VIEW_MODE_LIGHT_CLUSTER)
			{
				hashing = true;
				hashed = cluster + 1;
			}
			else
			{
				StructuredBuffer<uint> grid = ResourceDescriptorHeap[resolve_bindings.light_cluster_grid_];
				heat = saturate(grid[cluster * (1 + LIGHT_CLUSTER_MAX_LIGHTS)] / 16.0);
			}
		}
		else if (mode == VIEW_MODE_RAY_DIRECTION)
		{
			if (source == DEBUG_SOURCE_GLOBAL_ILLUMINATION)
			{
				StructuredBuffer<RaytracingGlobalIlluminationReservoir> reservoirs = ResourceDescriptorHeap[resolve_bindings.raytracing_global_illumination_reservoir_];
				float3 offset = reservoirs[reservoir_index].sample_position_ - world_position;
				color = dot(offset, offset) > 1e-8 ? normalize(offset) * 0.5 + 0.5 : float3(0.0, 0.0, 0.0);
			}
			else
			{
				StructuredBuffer<RaytracingReflectionReservoir> reservoirs = ResourceDescriptorHeap[resolve_bindings.raytracing_reflection_reservoir_];
				color = reservoirs[reservoir_index].sample_direction_ * 0.5 + 0.5;
			}
		}
		else if (mode >= VIEW_MODE_RESERVOIR_COUNT && mode <= VIEW_MODE_RESERVOIR_REJECTION)
		{
			float sample_m = 0.0;
			float sample_w = 0.0;
			float sample_age = 0.0;
			float3 sample_radiance = float3(0.0, 0.0, 0.0);
			float m_cap = 1.0;
			float max_age = 1.0;
			uint reuse = 0;
			if (source == DEBUG_SOURCE_GLOBAL_ILLUMINATION)
			{
				StructuredBuffer<RaytracingGlobalIlluminationReservoir> reservoirs = ResourceDescriptorHeap[resolve_bindings.raytracing_global_illumination_reservoir_];
				RaytracingGlobalIlluminationReservoir reservoir = reservoirs[reservoir_index];
				Texture2D<uint> reuse_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_global_illumination_reuse_];
				sample_m = reservoir.sample_m_;
				sample_w = reservoir.sample_w_;
				sample_age = reservoir.sample_age_;
				sample_radiance = reservoir.sample_radiance_;
				m_cap = GI_RESERVOIR_M_CAP;
				max_age = GI_RESERVOIR_MAX_AGE;
				reuse = reuse_texture.Load(int3(pixel, 0));
			}
			else
			{
				StructuredBuffer<RaytracingReflectionReservoir> reservoirs = ResourceDescriptorHeap[resolve_bindings.raytracing_reflection_reservoir_];
				RaytracingReflectionReservoir reservoir = reservoirs[reservoir_index];
				Texture2D<uint> reuse_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_reflection_reuse_];
				sample_m = reservoir.sample_m_;
				sample_w = reservoir.sample_w_;
				sample_age = reservoir.sample_age_;
				sample_radiance = reservoir.sample_radiance_;
				m_cap = REFLECTION_RESERVOIR_M_CAP;
				max_age = REFLECTION_RESERVOIR_MAX_AGE;
				reuse = reuse_texture.Load(int3(pixel, 0));
			}

			if (mode == VIEW_MODE_RESERVOIR_COUNT)
			{
				heat = saturate(sample_m / m_cap);
			}
			else if (mode == VIEW_MODE_RESERVOIR_WEIGHT)
			{
				heat = saturate((log2(max(sample_w, 1e-6)) + 8.0) / 16.0);
			}
			else if (mode == VIEW_MODE_RESERVOIR_AGE)
			{
				heat = saturate(sample_age / max_age);
			}
			else if (mode == VIEW_MODE_RESERVOIR_RADIANCE)
			{
				color = sample_radiance / (1.0 + sample_radiance);
			}
			else if (mode == VIEW_MODE_RESERVOIR_TEMPORAL)
			{
				color = (reuse & DEBUG_REUSE_TEMPORAL) != 0 ? float3(0.2, 0.85, 0.35) : float3(0.15, 0.15, 0.15);
			}
			else if (mode == VIEW_MODE_RESERVOIR_SPATIAL)
			{
				heat = saturate((reuse >> DEBUG_REUSE_SPATIAL_SHIFT) / 8.0);
			}
			else
			{
				color = (reuse & DEBUG_REUSE_REJECTED) != 0 ? float3(0.9, 0.25, 0.2) : float3(0.15, 0.15, 0.15);
			}
		}
		else if (mode >= VIEW_MODE_DENOISER_RAW && mode <= VIEW_MODE_DENOISER_REPROJECTION)
		{
			float3 raw = float3(0.0, 0.0, 0.0);
			float3 denoised = float3(0.0, 0.0, 0.0);
			float deviation = 0.0;
			float history_length = 0.0;
			float2 validity = float2(0.0, 0.0);
			if (source == DEBUG_SOURCE_GLOBAL_ILLUMINATION)
			{
				Texture2D<float4> raw_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_global_illumination_raw_];
				Texture2D<float4> denoised_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_global_illumination_radiance_];
				Texture2D<float> variance_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_global_illumination_variance_];
				Texture2D<float> history_length_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_global_illumination_history_length_];
				Texture2D<float2> validity_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_global_illumination_validity_];
				raw = raw_texture.Load(int3(pixel, 0)).rgb;
				denoised = denoised_texture.Load(int3(pixel, 0)).rgb;
				deviation = sqrt(max(variance_texture.Load(int3(pixel, 0)), 0.0));
				history_length = history_length_texture.Load(int3(pixel, 0));
				validity = validity_texture.Load(int3(pixel, 0));
			}
			else if (source == DEBUG_SOURCE_SHADOW)
			{
				Texture2D<float2> raw_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_shadow_raw_];
				Texture2D<float2> denoised_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_shadow_visibility_];
				Texture2D<float4> moments_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_shadow_moments_];
				Texture2D<float> history_length_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_shadow_history_length_];
				Texture2D<float2> validity_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_shadow_validity_];
				float4 moments = moments_texture.Load(int3(pixel, 0));
				raw = float3(raw_texture.Load(int3(pixel, 0)), 0.0);
				denoised = float3(denoised_texture.Load(int3(pixel, 0)), 0.0);
				deviation = sqrt(max(max(moments.y - moments.x * moments.x, moments.w - moments.z * moments.z), 0.0));
				history_length = history_length_texture.Load(int3(pixel, 0));
				validity = validity_texture.Load(int3(pixel, 0));
			}
			else if (source == DEBUG_SOURCE_AMBIENT_OCCLUSION)
			{
				Texture2D<float> raw_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_ambient_occlusion_raw_];
				Texture2D<float> denoised_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_ambient_occlusion_openness_];
				Texture2D<float> variance_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_ambient_occlusion_variance_];
				Texture2D<float> history_length_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_ambient_occlusion_history_length_];
				Texture2D<float2> validity_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_ambient_occlusion_validity_];
				raw = raw_texture.Load(int3(pixel, 0)).xxx;
				denoised = denoised_texture.Load(int3(pixel, 0)).xxx;
				deviation = sqrt(max(variance_texture.Load(int3(pixel, 0)), 0.0)) * 4.0;
				history_length = history_length_texture.Load(int3(pixel, 0));
				validity = validity_texture.Load(int3(pixel, 0));
			}
			else
			{
				Texture2D<float4> raw_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_reflection_raw_];
				Texture2D<float4> denoised_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_reflection_radiance_];
				Texture2D<float2> moments_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_reflection_moments_];
				Texture2D<float> history_length_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_reflection_history_length_];
				Texture2D<float2> validity_texture = ResourceDescriptorHeap[resolve_bindings.raytracing_reflection_validity_];
				float2 moments = moments_texture.Load(int3(pixel, 0));
				raw = raw_texture.Load(int3(pixel, 0)).rgb;
				denoised = denoised_texture.Load(int3(pixel, 0)).rgb;
				deviation = sqrt(max(moments.y - moments.x * moments.x, 0.0));
				history_length = history_length_texture.Load(int3(pixel, 0));
				validity = validity_texture.Load(int3(pixel, 0));
			}

			if (mode == VIEW_MODE_DENOISER_RAW)
			{
				color = raw / (1.0 + raw);
			}
			else if (mode == VIEW_MODE_DENOISER_DENOISED)
			{
				color = denoised / (1.0 + denoised);
			}
			else if (mode == VIEW_MODE_DENOISER_VARIANCE)
			{
				heat = saturate(deviation);
			}
			else if (mode == VIEW_MODE_DENOISER_LENGTH)
			{
				heat = saturate(history_length / 32.0);
			}
			else if (mode == VIEW_MODE_DENOISER_VALIDITY)
			{
				color = validity.x > 0.5 ? float3(0.2, 0.85, 0.35) : float3(0.9, 0.25, 0.2);
			}
			else
			{
				heat = saturate(validity.y / 16.0);
			}
		}
		else if (mode == VIEW_MODE_SHADER_COMPLEXITY && surface)
		{
			float cost = 1.0;
			cost += instance.texture_.base_color_texture_index_ != MODEL_INVALID ? 1.0 : 0.0;
			cost += instance.texture_.normal_texture_index_ != MODEL_INVALID ? 1.0 : 0.0;
			cost += instance.texture_.metallic_roughness_texture_index_ != MODEL_INVALID ? 1.0 : 0.0;
			cost += instance.texture_.emissive_texture_index_ != MODEL_INVALID ? 1.0 : 0.0;
			cost += instance.texture_.occlusion_texture_index_ != MODEL_INVALID ? 1.0 : 0.0;
			cost += instance.extension_.clearcoat_factor_ > 0.0 ? 2.0 : 0.0;
			cost += any(instance.extension_.sheen_color_ > 0.0) ? 2.0 : 0.0;
			cost += instance.extension_.transmission_factor_ > 0.0 ? 3.0 : 0.0;
			cost += instance.extension_.iridescence_factor_ > 0.0 ? 2.0 : 0.0;
			cost += instance.extension_.anisotropy_ != 0.0 ? 2.0 : 0.0;
			cost += instance.shading_.material_mapping_ != 0 ? 2.0 : 0.0;
			cost += instance.shading_.shading_model_ != 0 ? 1.0 : 0.0;
			cost += instance.geometry_.layer_ != 0 ? 2.0 : 0.0;
			heat = saturate(cost / 16.0);
		}
		else if (mode == VIEW_MODE_QUAD_OVERDRAW && surface)
		{
			uint2 quad = pixel & ~1u;
			uint2 quad_ids[4];
			uint distinct = 0;
			[unroll]
			for (uint quad_index = 0; quad_index < 4; quad_index++)
			{
				uint2 quad_pixel = min(quad + uint2(quad_index & 1, quad_index >> 1), uint2(render_width - 1, render_height - 1));
				quad_ids[quad_index] = visibility_id.Load(int3(quad_pixel, 0));
				bool seen = quad_ids[quad_index].x == 0;
				for (uint previous_index = 0; previous_index < quad_index; previous_index++)
				{
					seen = seen || all(quad_ids[previous_index] == quad_ids[quad_index]);
				}
				distinct += seen ? 0 : 1;
			}
			heat = saturate((distinct - 1.0) / 3.0);
		}
		else if ((mode == VIEW_MODE_TRIANGLE_DENSITY || mode == VIEW_MODE_MESHLET_DENSITY) && surface)
		{
			uint2 tile = pixel & ~3u;
			uint2 tile_ids[16];
			uint distinct = 0;
			for (uint tile_index = 0; tile_index < 16; tile_index++)
			{
				uint2 tile_pixel = min(tile + uint2(tile_index & 3, tile_index >> 2), uint2(render_width - 1, render_height - 1));
				uint2 tile_id = visibility_id.Load(int3(tile_pixel, 0));
				if (mode == VIEW_MODE_MESHLET_DENSITY)
				{
					tile_id.y >>= 7;
				}
				tile_ids[tile_index] = tile_id;
				bool seen = tile_id.x == 0;
				for (uint previous_index = 0; previous_index < tile_index; previous_index++)
				{
					seen = seen || all(tile_ids[previous_index] == tile_id);
				}
				distinct += seen ? 0 : 1;
			}
			heat = saturate(distinct / 16.0);
		}
		else if (mode == VIEW_MODE_TEXTURE_MIP && surface)
		{
			Texture2D<float> mip = ResourceDescriptorHeap[resolve_bindings.material_mip_];
			float level = mip.Load(int3(pixel, 0));
			if (level <= 0.0)
			{
				color = float3(0.3, 0.3, 0.3);
			}
			else
			{
				heat = saturate((level - 1.0) / 10.0);
			}
		}
	}

	if (heat >= 0.0)
	{
		color = saturate(float3(1.5 - abs(4.0 * heat - 3.0), 1.5 - abs(4.0 * heat - 2.0), 1.5 - abs(4.0 * heat - 1.0)));
	}
	if (hashing)
	{
		uint hash = hashed * 747796405u + 2891336453u;
		hash = ((hash >> ((hash >> 28) + 4)) ^ hash) * 277803737u;
		hash = (hash >> 22) ^ hash;
		color = float3(hash & 0xFF, (hash >> 8) & 0xFF, (hash >> 16) & 0xFF) / 255.0;
	}
	display_color[id.xy] = float4(color, 1.0);
}

/**
* [EN]
* Meshlet bounds view mode: one thread per entry of the view's draw list
* (early and late). Projects the meshlet's bounding sphere to the display and
* draws its outline into DisplayColor over the lit image, in a color hashed
* from the entry so neighbouring meshlets stay apart.
*/
[numthreads(64, 1, 1)]
void Bounds(uint3 id : SV_DispatchThreadID)
{
	StructuredBuffer<uint2> draw_lists = ResourceDescriptorHeap[bounds_bindings.culling_draw_lists_];
	StructuredBuffer<uint> arguments = ResourceDescriptorHeap[bounds_bindings.culling_draw_arguments_];
	uint capacity;
	uint stride;
	draw_lists.GetDimensions(capacity, stride);
	uint early_count = min(arguments[CULLING_EARLY_COUNT], capacity);
	uint count = early_count + min(arguments[CULLING_LATE_COUNT], capacity - early_count);
	if (id.x >= count)
	{
		return;
	}

	uint2 entry = draw_lists[id.x];
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bounds_bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[entry.x];
	StructuredBuffer<ModelMeshletBound> bounds = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.meshlet_bound_buffer_index_)];
	ModelMeshletBound bound = bounds[entry.y];
	float world_scale = max(max(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz)), length(instance.transform_.world_[2].xyz));
	float3 center = mul(float4(bound.center_, 1.0), instance.transform_.world_).xyz;
	float radius = bound.radius_ * world_scale;

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	float view_depth = mul(float4(center, 1.0), scene.view_).z;
	if (view_depth <= scene.near_plane_)
	{
		return;
	}

	RWTexture2D<float4> display_color = ResourceDescriptorHeap[bounds_bindings.hud_display_color_];
	uint display_width;
	uint display_height;
	display_color.GetDimensions(display_width, display_height);
	float4 clip = mul(float4(center, 1.0), scene.current_view_projection_);
	float2 ndc = clip.xy / clip.w;
	float2 screen = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5) * float2(display_width, display_height);
	float pixel_radius = radius / view_depth * scene.projection_._m11 * display_height * 0.5;
	if (pixel_radius < 1.0 || pixel_radius > float(max(display_width, display_height)))
	{
		return;
	}

	uint hash = entry.x * 747796405u + entry.y * 2891336453u;
	hash = ((hash >> ((hash >> 28) + 4)) ^ hash) * 277803737u;
	hash = (hash >> 22) ^ hash;
	float4 outline = float4(float3(hash & 0xFF, (hash >> 8) & 0xFF, (hash >> 16) & 0xFF) / 255.0, 1.0);

	uint segment_count = clamp(uint(pixel_radius * 6.28318530718), 8u, 2048u);
	for (uint segment = 0; segment < segment_count; segment++)
	{
		float angle = (float)segment / (float)segment_count * 6.28318530718;
		float2 point_position = screen + float2(cos(angle), sin(angle)) * pixel_radius;
		if (all(point_position >= 0.0) && point_position.x < display_width && point_position.y < display_height)
		{
			display_color[uint2(point_position)] = outline;
		}
	}
}
