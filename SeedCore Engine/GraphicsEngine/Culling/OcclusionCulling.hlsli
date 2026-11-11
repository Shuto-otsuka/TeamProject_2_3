#ifndef __OCCLUSION_CULLING_HLSL__
#define __OCCLUSION_CULLING_HLSL__

/**
* [EN]
* Sphere versus a Hi-Z pyramid. Each texel stores the MINIMUM depth of its
* footprint, which under reversed-Z is the farthest surface there; the sphere
* is occluded when even its nearest point is farther than all of it. The mip
* is chosen so the projected sphere covers about 2x2 texels, and the four
* corner texels are compared. Every uncertain case (camera inside the sphere,
* sphere crossing the camera plane) counts as visible.
*/
bool IsOccluded(float3 center, float radius, row_major float4x4 view_projection, float3 camera_position, float projection_scale_y, float2 screen_size, uint hierarchical_depth)
{
	float3 to_camera = camera_position - center;
	float center_distance = length(to_camera);
	if (center_distance <= radius)
	{
		return false;
	}

	float3 nearest = center + to_camera / center_distance * radius;
	float4 nearest_clip = mul(float4(nearest, 1.0), view_projection);
	float4 center_clip = mul(float4(center, 1.0), view_projection);
	if (nearest_clip.w <= 0.0 || center_clip.w <= 0.0)
	{
		return false;
	}
	float nearest_depth = nearest_clip.z / nearest_clip.w;

	float2 center_ndc = center_clip.xy / center_clip.w;
	float2 center_pixel = float2(center_ndc.x * 0.5 + 0.5, 0.5 - center_ndc.y * 0.5) * screen_size;
	float radius_pixels = radius * projection_scale_y * screen_size.y * 0.5 / center_clip.w;

	Texture2D<float> depth_pyramid = ResourceDescriptorHeap[hierarchical_depth];
	uint pyramid_width;
	uint pyramid_height;
	uint pyramid_mip_count;
	depth_pyramid.GetDimensions(0, pyramid_width, pyramid_height, pyramid_mip_count);

	uint mip_level = min((uint)max(ceil(log2(max(radius_pixels * 2.0, 1.0))), 0.0), pyramid_mip_count - 1);
	float2 mip_size = float2(max(pyramid_width >> mip_level, 1u), max(pyramid_height >> mip_level, 1u));

	float2 uv_min = saturate((center_pixel - radius_pixels) / screen_size);
	float2 uv_max = saturate((center_pixel + radius_pixels) / screen_size);
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
