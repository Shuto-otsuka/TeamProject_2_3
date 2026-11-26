#ifndef __FROXEL_HLSL__
#define __FROXEL_HLSL__

static const float FROXEL_DEPTH_DISTRIBUTION = 256.0;

float FroxelSliceToViewZ(float slice_t, float near_plane, float far_plane)
{
	return near_plane + (far_plane - near_plane) * (pow(FROXEL_DEPTH_DISTRIBUTION, slice_t) - 1.0) / (FROXEL_DEPTH_DISTRIBUTION - 1.0);
}

float ViewZToFroxelSlice(float view_z, float near_plane, float far_plane)
{
	return log2(1.0 + max(view_z - near_plane, 0.0) * (FROXEL_DEPTH_DISTRIBUTION - 1.0) / (far_plane - near_plane)) / log2(FROXEL_DEPTH_DISTRIBUTION);
}

float3 FroxelToWorld(uint3 froxel, uint3 froxel_dimensions, float near_plane, float far_plane, float4x4 inverse_view_projection)
{
	float2 uv = (float2(froxel.xy) + 0.5) / float2(froxel_dimensions.xy);
	float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);

	float slice_t = (float(froxel.z) + 0.5) / float(froxel_dimensions.z);
	float view_z = FroxelSliceToViewZ(slice_t, near_plane, far_plane);

	float device_depth = near_plane / view_z;
	float4 clip = float4(ndc, device_depth, 1.0);
	float4 world = mul(clip, inverse_view_projection);
	return world.xyz / world.w;
}

float3 WorldToFroxelUVW(float3 world_position, float4x4 view, float near_plane, float far_plane)
{
	float4 view_position = mul(float4(world_position, 1.0), view);
	float view_z = view_position.z;

	float slice_t = saturate(ViewZToFroxelSlice(view_z, near_plane, far_plane));
	return float3(0.0, 0.0, slice_t);
}

float PhaseHenyeyGreenstein(float cos_theta, float g)
{
	const float PI = 3.14159265358979;
	float g2 = g * g;
	float denom = 1.0 + g2 - 2.0 * g * cos_theta;
	return (1.0 - g2) / (4.0 * PI * pow(max(denom, 1e-4), 1.5));
}

#endif
