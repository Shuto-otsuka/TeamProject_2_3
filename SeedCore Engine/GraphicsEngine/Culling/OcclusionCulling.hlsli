#ifndef __OCCLUSION_CULLING_HLSL__
#define __OCCLUSION_CULLING_HLSL__

/**
* [EN]
* Sphere versus a Hi-Z pyramid. Each texel stores the MINIMUM depth of its
* footprint, which under reversed-Z is the farthest surface there; the sphere
* is occluded when even its nearest point is farther than all of it. The
* sphere is bounded by its axis-aligned box, whose eight corners are
* projected: their screen rectangle contains the sphere's projection however
* close or off-center it is, and their largest depth is at least the
* sphere's nearest depth, since view depth is linear over the box and peaks
* at a corner. The mip is chosen so the rectangle covers at most 2x2 texels,
* and the four corner texels are compared. A corner behind the camera (the
* camera inside the box or the box crossing the camera plane) counts as
* visible.
*/
bool IsOccluded(float3 center, float radius, row_major float4x4 view_projection, uint hi_z)
{
	float2 ndc_min = float2(3.402823e38, 3.402823e38);
	float2 ndc_max = float2(-3.402823e38, -3.402823e38);
	float nearest_depth = 0.0;
	[unroll]
	for (uint corner_index = 0; corner_index < 8; corner_index++)
	{
		float3 corner = center + float3((corner_index & 1) != 0 ? radius : -radius, (corner_index & 2) != 0 ? radius : -radius, (corner_index & 4) != 0 ? radius : -radius);
		float4 clip = mul(float4(corner, 1.0), view_projection);
		if (clip.w <= 0.0)
		{
			return false;
		}
		float3 ndc = clip.xyz / clip.w;
		ndc_min = min(ndc_min, ndc.xy);
		ndc_max = max(ndc_max, ndc.xy);
		nearest_depth = max(nearest_depth, ndc.z);
	}

	float2 uv_min = saturate(float2(ndc_min.x, -ndc_max.y) * 0.5 + 0.5);
	float2 uv_max = saturate(float2(ndc_max.x, -ndc_min.y) * 0.5 + 0.5);

	Texture2D<float> depth_pyramid = ResourceDescriptorHeap[hi_z];
	uint pyramid_width;
	uint pyramid_height;
	uint pyramid_mip_count;
	depth_pyramid.GetDimensions(0, pyramid_width, pyramid_height, pyramid_mip_count);

	float2 extent = (uv_max - uv_min) * float2(pyramid_width, pyramid_height);
	uint mip_level = min((uint)ceil(log2(max(max(extent.x, extent.y), 1.0))), pyramid_mip_count - 1);
	float2 mip_size = float2(max(pyramid_width >> mip_level, 1u), max(pyramid_height >> mip_level, 1u));

	int2 texel_min = int2(clamp(uv_min * mip_size, 0.0, mip_size - 1.0));
	int2 texel_max = int2(clamp(uv_max * mip_size, 0.0, mip_size - 1.0));

	float depth00 = depth_pyramid.Load(int3(texel_min.x, texel_min.y, mip_level));
	float depth10 = depth_pyramid.Load(int3(texel_max.x, texel_min.y, mip_level));
	float depth01 = depth_pyramid.Load(int3(texel_min.x, texel_max.y, mip_level));
	float depth11 = depth_pyramid.Load(int3(texel_max.x, texel_max.y, mip_level));
	float footprint_depth = min(min(depth00, depth10), min(depth01, depth11));

	return nearest_depth < footprint_depth;
}

#endif // __OCCLUSION_CULLING_HLSL__
