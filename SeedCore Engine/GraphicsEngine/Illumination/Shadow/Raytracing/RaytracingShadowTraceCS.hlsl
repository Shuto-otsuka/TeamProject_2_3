#include <GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadow.hlsli>
#include <GraphicsEngine/Raytracing/RaytracingHit.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

struct RaytracingShadowTraceBindings
{
	uint geometry_depth_;
	uint material_normal_roughness_;
	uint light_list_;
	uint light_constants_;
	uint light_clusters_;
	uint top_level_structure_;
	uint top_level_hits_;
	uint raytracing_shadow_constants_;
	uint raytracing_shadow_raw_;
	uint debug_view_rays_;
};
ConstantBuffer<RaytracingShadowTraceBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<float2> raw = ResourceDescriptorHeap[bindings.raytracing_shadow_raw_];
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
		raw[pixel] = float2(1.0, 1.0);
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<RaytracingShadowConstantBuffer> constants = ResourceDescriptorHeap[bindings.raytracing_shadow_constants_];
	ConstantBuffer<LightConstantBuffer> light_constants = ResourceDescriptorHeap[bindings.light_constants_];

	float2 uv = (float2(pixel) + 0.5) / float2(width, height);
	float4 world = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), scene.inverse_view_projection_);
	float3 world_position = world.xyz / world.w;
	float view_depth = mul(float4(world_position, 1.0), scene.view_).z;

	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_normal_roughness_];
	uint normal_roughness = surface_normal_roughness.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(float2(normal_roughness & 0xFFF, (normal_roughness >> 12) & 0xFFF) / 4095.0);
	float3 origin = world_position + normal * constants.normal_bias_;
	uint random_state = SeedFromPixel(pixel, constants.frame_);

	uint2 tile_count = (uint2(width, height) + LIGHT_CLUSTER_TILE_SIZE - 1) / LIGHT_CLUSTER_TILE_SIZE;
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

	StructuredBuffer<uint> grid = ResourceDescriptorHeap[bindings.light_clusters_];
	StructuredBuffer<LightStructuredBuffer> lights = ResourceDescriptorHeap[bindings.light_list_];
	uint base = cluster * (1 + LIGHT_CLUSTER_MAX_LIGHTS);
	uint light_count = grid[base];

	uint picked = 0xFFFFFFFF;
	float weight_sum = 0.0;
	for (uint light_slot = 0; light_slot < light_count; light_slot++)
	{
		uint light_index = grid[base + 1 + light_slot];
		LightStructuredBuffer light = lights[light_index];

		float3 light_position = light.position_;
		if (light.type_ == LIGHT_TYPE_RECT)
		{
			float3 offset = world_position - light.position_;
			if (dot(offset, light.direction_) <= 0.0)
			{
				continue;
			}
			light_position += light.right_ * clamp(dot(offset, light.right_), -light.half_width_, light.half_width_);
			light_position += light.up_ * clamp(dot(offset, light.up_), -light.half_height_, light.half_height_);
		}

		float3 to_light = light_position - world_position;
		float distance_to_light = length(to_light);
		float3 light_direction = to_light / max(distance_to_light, 0.0001);
		if (dot(normal, light_direction) <= 0.0)
		{
			continue;
		}

		float attenuation_ratio = saturate(distance_to_light / light.range_);
		float attenuation_ratio2 = attenuation_ratio * attenuation_ratio;
		float attenuation = saturate(1.0 - attenuation_ratio2 * attenuation_ratio2);
		attenuation = attenuation * attenuation / max(distance_to_light * distance_to_light, 0.0001);
		if (light.type_ == LIGHT_TYPE_SPOT)
		{
			float cos_angle = dot(-light_direction, light.direction_);
			attenuation *= saturate((cos_angle - light.cos_half_angle_) / max(light.softness_ * (1.0 - light.cos_half_angle_), 0.0001));
		}

		float weight = dot(light.color_, float3(0.2126, 0.7152, 0.0722)) * light.intensity_ * attenuation;
		if (weight > 0.0)
		{
			weight_sum += weight;
			if (Rand(random_state) * weight_sum <= weight)
			{
				picked = light_index;
			}
		}
	}

	float3 sun_direction = normalize(-light_constants.sun_direction_);
	bool sun_present = light_constants.sun_intensity_ > 0.0;
	bool sun_facing = sun_present && dot(normal, sun_direction) > 0.0;
	bool punctual_present = picked != 0xFFFFFFFF;
	LightStructuredBuffer picked_light = lights[punctual_present ? picked : 0];

	RaytracingHitTables tables;
	tables.top_level_ = bindings.top_level_structure_;
	tables.hits_ = bindings.top_level_hits_;

	uint ray_count = max(constants.ray_count_, 1);
	float sun_visible = 0.0;
	float punctual_visible = 0.0;
	for (uint ray_index = 0; ray_index < ray_count * 2; ray_index++)
	{
		bool sun_ray = ray_index < ray_count;
		if (sun_ray ? !sun_facing : !punctual_present)
		{
			continue;
		}

		RayDesc ray;
		ray.Origin = origin;
		ray.TMin = 0.001;
		if (sun_ray)
		{
			ray.Direction = SampleCone(random_state, sun_direction, cos(constants.sun_angular_radius_));
			ray.TMax = constants.ray_t_max_;
		}
		else
		{
			float3 sample_point = picked_light.position_;
			float light_radius = constants.punctual_radius_;
			if (picked_light.type_ == LIGHT_TYPE_RECT)
			{
				float2 rect_uv = Rand2(random_state) * 2.0 - 1.0;
				sample_point += picked_light.right_ * (rect_uv.x * picked_light.half_width_) + picked_light.up_ * (rect_uv.y * picked_light.half_height_);
				light_radius = 0.0;
			}
			float3 to_light = sample_point - world_position;
			float distance_to_light = length(to_light);
			float3 light_direction = to_light / max(distance_to_light, 0.0001);
			float sin_theta_max = saturate(light_radius / max(distance_to_light, 0.001));
			ray.Direction = light_radius > 0.0 ? SampleCone(random_state, light_direction, sqrt(max(0.0, 1.0 - sin_theta_max * sin_theta_max))) : light_direction;
			ray.TMax = max(distance_to_light - 0.01, 0.001);
		}

		float visible = RaytracingHitIsOccluded(tables, ray) ? 0.0 : 1.0;
		if (scene.view_mode_ == VIEW_MODE_RAY_COUNT)
		{
			RWTexture2D<uint> ray_count_texture = ResourceDescriptorHeap[bindings.debug_view_rays_];
			InterlockedAdd(ray_count_texture[pixel], 1);
		}
		if (sun_ray)
		{
			sun_visible += visible;
		}
		else
		{
			punctual_visible += visible;
		}
	}

	float sun = sun_present ? (sun_facing ? sun_visible / ray_count : 0.0) : 1.0;
	float punctual = punctual_present ? punctual_visible / ray_count : 1.0;
	float strength = max(constants.strength_, 0.0);
	raw[pixel] = float2(saturate(lerp(1.0, sun, strength)), saturate(lerp(1.0, punctual, strength)));
}
