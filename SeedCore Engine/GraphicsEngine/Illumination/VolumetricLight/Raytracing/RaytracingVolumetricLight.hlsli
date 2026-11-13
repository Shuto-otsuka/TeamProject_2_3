#ifndef __RAYTRACING_VOLUMETRIC_LIGHT_HLSLI__
#define __RAYTRACING_VOLUMETRIC_LIGHT_HLSLI__

#include <GraphicsEngine/Illumination/VolumetricLight/Froxel.hlsli>

struct RaytracingVolumetricLightConstantBuffer
{
	float density_;
	float absorption_;
	float height_falloff_;
	float height_reference_;

	float3 fog_albedo_;
	float scattering_g_;

	float3 jitter_;
	float ray_t_max_;

	float godray_strength_;
	uint frame_;
	uint history_valid_;
	uint raytracing_volumetric_light_constant_buffer_padding_0_;
};

float3 RaytracingVolumetricLightFroxelToWorld(float3 froxel_coordinate, uint3 froxel_dimensions, float near_plane, float far_plane, float4x4 projection, float4x4 inverse_view)
{
	float2 uv = froxel_coordinate.xy / float2(froxel_dimensions.xy);
	float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
	float view_z = FroxelSliceToViewZ(froxel_coordinate.z / float(froxel_dimensions.z), near_plane, far_plane);
	float3 view_position = float3(ndc.x * view_z / projection[0][0], ndc.y * view_z / projection[1][1], view_z);
	return mul(float4(view_position, 1.0), inverse_view).xyz;
}

#endif // __RAYTRACING_VOLUMETRIC_LIGHT_HLSLI__
