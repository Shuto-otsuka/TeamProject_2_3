#include <GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLight.hlsli>
#include <GraphicsEngine/Raytracing/RaytracingHit.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>

static const float FROXEL_TEMPORAL_ALPHA = 0.1;

struct RaytracingVolumetricLightScatterBindings
{
	uint raytracing_volumetric_light_density_;
	uint light_constants_;
	uint sky_diffuse_;
	uint top_level_structure_;
	uint top_level_hits_;
	uint raytracing_volumetric_light_constants_;
	uint raytracing_volumetric_light_scattering_history_;
	uint raytracing_volumetric_light_scattering_;
};
ConstantBuffer<RaytracingVolumetricLightScatterBindings> bindings : register(b0, space1);

[numthreads(4, 4, 4)]
void main(uint3 id : SV_DispatchThreadID)
{
	RWTexture3D<float4> scattering_volume = ResourceDescriptorHeap[bindings.raytracing_volumetric_light_scattering_];
	uint3 froxel_dimensions;
	scattering_volume.GetDimensions(froxel_dimensions.x, froxel_dimensions.y, froxel_dimensions.z);
	if (any(id >= froxel_dimensions))
	{
		return;
	}

	Texture3D<float4> density_volume = ResourceDescriptorHeap[bindings.raytracing_volumetric_light_density_];
	float4 medium = density_volume.Load(int4(id, 0));
	float3 scattering_coefficient = medium.rgb;
	float extinction = medium.a;
	if (all(scattering_coefficient <= 0.0))
	{
		scattering_volume[id] = float4(0.0, 0.0, 0.0, extinction);
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<RaytracingVolumetricLightConstantBuffer> constants = ResourceDescriptorHeap[bindings.raytracing_volumetric_light_constants_];
	ConstantBuffer<LightConstantBuffer> light_constants = ResourceDescriptorHeap[bindings.light_constants_];
	float3 world_position = RaytracingVolumetricLightFroxelToWorld(float3(id) + 0.5 + constants.jitter_, froxel_dimensions, scene.near_plane_, scene.far_plane_, scene.projection_, scene.inverse_view_);
	float3 view_direction = normalize(world_position - scene.camera_position_.xyz);

	TextureCube<float4> diffuse_irradiance = ResourceDescriptorHeap[bindings.sky_diffuse_];
	float3 in_scattering = scattering_coefficient * diffuse_irradiance.SampleLevel(sampler_linear_clamp, float3(0.0, 1.0, 0.0), 0).rgb * light_constants.sky_intensity_;

	if (light_constants.sun_intensity_ > 0.0)
	{
		float3 sun_direction = normalize(-light_constants.sun_direction_);
		uint random_state = SeedFromPixel(id.xy, constants.frame_ + id.z * 7919u);

		RaytracingHitTables tables;
		tables.top_level_ = bindings.top_level_structure_;
		tables.hits_ = bindings.top_level_hits_;

		RayDesc ray;
		ray.Origin = world_position;
		ray.Direction = SampleCone(random_state, sun_direction, cos(light_constants.sun_angular_radius_));
		ray.TMin = 0.001;
		ray.TMax = constants.ray_t_max_;
		float3 transmittance = RaytracingHitTransmittance(tables, ray);
		float phase = PhaseHenyeyGreenstein(dot(view_direction, sun_direction), constants.scattering_g_);
		in_scattering += scattering_coefficient * phase * transmittance * light_constants.sun_color_ * light_constants.sun_intensity_ * constants.godray_strength_;
	}

	float4 scattering = float4(in_scattering, extinction);
	if (constants.history_valid_ != 0)
	{
		float3 center_position = RaytracingVolumetricLightFroxelToWorld(float3(id) + 0.5, froxel_dimensions, scene.near_plane_, scene.far_plane_, scene.projection_, scene.inverse_view_);
		float4 previous_clip = mul(float4(center_position, 1.0), scene.previous_non_jitter_view_projection_);
		if (previous_clip.w > 0.0)
		{
			float2 previous_ndc = previous_clip.xy / previous_clip.w;
			float3 previous_uvw = float3(previous_ndc.x * 0.5 + 0.5, 0.5 - previous_ndc.y * 0.5, ViewZToFroxelSlice(previous_clip.w, scene.near_plane_, scene.far_plane_));
			if (all(previous_uvw >= 0.0) && all(previous_uvw <= 1.0))
			{
				Texture3D<float4> scattering_history = ResourceDescriptorHeap[bindings.raytracing_volumetric_light_scattering_history_];
				scattering = lerp(scattering_history.SampleLevel(sampler_linear_clamp, previous_uvw, 0), scattering, FROXEL_TEMPORAL_ALPHA);
			}
		}
	}
	scattering_volume[id] = scattering;
}
