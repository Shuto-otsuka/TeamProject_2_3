#ifndef __FRUSTUM_CULLING_HLSL__
#define __FRUSTUM_CULLING_HLSL__

/**
* [EN]
* Sphere versus view frustum. The six planes are taken straight from the
* row-vector view-projection matrix; the two depth planes are near/far in
* either order, so the test holds for both standard and reversed-Z
* projections. Returns true when any part of the sphere is inside.
*/
bool IsInsideFrustum(float3 center, float radius, row_major float4x4 view_projection)
{
	float4 planes[6];
	planes[0] = float4(view_projection[0][0] + view_projection[0][3], view_projection[1][0] + view_projection[1][3], view_projection[2][0] + view_projection[2][3], view_projection[3][0] + view_projection[3][3]);
	planes[1] = float4(view_projection[0][3] - view_projection[0][0], view_projection[1][3] - view_projection[1][0], view_projection[2][3] - view_projection[2][0], view_projection[3][3] - view_projection[3][0]);
	planes[2] = float4(view_projection[0][1] + view_projection[0][3], view_projection[1][1] + view_projection[1][3], view_projection[2][1] + view_projection[2][3], view_projection[3][1] + view_projection[3][3]);
	planes[3] = float4(view_projection[0][3] - view_projection[0][1], view_projection[1][3] - view_projection[1][1], view_projection[2][3] - view_projection[2][1], view_projection[3][3] - view_projection[3][1]);
	planes[4] = float4(view_projection[0][2], view_projection[1][2], view_projection[2][2], view_projection[3][2]);
	planes[5] = float4(view_projection[0][3] - view_projection[0][2], view_projection[1][3] - view_projection[1][2], view_projection[2][3] - view_projection[2][2], view_projection[3][3] - view_projection[3][2]);

	[unroll]
	for (uint plane_index = 0; plane_index < 6; plane_index++)
	{
		if (dot(planes[plane_index].xyz, center) + planes[plane_index].w < -radius * length(planes[plane_index].xyz))
		{
			return false;
		}
	}
	return true;
}

#endif // __FRUSTUM_CULLING_HLSL__
