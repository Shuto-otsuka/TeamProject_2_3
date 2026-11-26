#ifndef __BACKFACE_CULLING_HLSL__
#define __BACKFACE_CULLING_HLSL__

bool IsBackfacing(float3 center, float radius, float3 cone_axis, float cone_cutoff, float3 camera_position)
{
	if (cone_cutoff <= 0.0)
	{
		return false;
	}
	float3 camera_to_center = center - camera_position;
	float cone_sine = sqrt(1.0 - cone_cutoff * cone_cutoff);
	return dot(camera_to_center, cone_axis) >= cone_sine * length(camera_to_center) + radius;
}

bool IsBackfacing(float4 clip0, float4 clip1, float4 clip2)
{
	return determinant(float3x3(clip0.xyw, clip1.xyw, clip2.xyw)) <= 0.0;
}

#endif
