#ifndef __BACKFACE_CULLING_HLSL__
#define __BACKFACE_CULLING_HLSL__

/**
* [EN]
* Normal-cone test for a cluster of triangles (a meshlet). cone_axis is the
* world-space average facing and cone_cutoff the cosine bound of the cone;
* every triangle faces away from the camera when the camera lies behind the
* cone, including its bounding-sphere slack. A cutoff of 0 or less means the
* cone is too wide to decide, so nothing is rejected.
*/
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

/**
* [EN]
* Screen-space facing of one triangle from its clip positions. Front faces
* are counter-clockwise in NDC; triangles crossing the w = 0 plane are kept.
*/
bool IsBackfacing(float4 clip0, float4 clip1, float4 clip2)
{
	return determinant(float3x3(clip0.xyw, clip1.xyw, clip2.xyw)) <= 0.0;
}

#endif // __BACKFACE_CULLING_HLSL__
