#ifndef __OCCLUSION_CULLING_HLSL__
#define __OCCLUSION_CULLING_HLSL__

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

	uint2 pyramid_size = uint2(pyramid_width, pyramid_height);
	uint2 base_min = min(uint2(uv_min * float2(pyramid_size)), pyramid_size - 1);
	uint2 base_max = min(uint2(uv_max * float2(pyramid_size)) + 1, pyramid_size - 1);
	uint2 base_extent = base_max - base_min;
	uint mip_level = min((uint)ceil(log2(max((float)max(base_extent.x, base_extent.y), 1.0))), pyramid_mip_count - 1);
	uint2 mip_size = max(pyramid_size >> mip_level, uint2(1, 1));

	int2 texel_min = int2(min(base_min >> mip_level, mip_size - 1));
	int2 texel_max = int2(min(base_max >> mip_level, mip_size - 1));

	float depth00 = depth_pyramid.Load(int3(texel_min.x, texel_min.y, mip_level));
	float depth10 = depth_pyramid.Load(int3(texel_max.x, texel_min.y, mip_level));
	float depth01 = depth_pyramid.Load(int3(texel_min.x, texel_max.y, mip_level));
	float depth11 = depth_pyramid.Load(int3(texel_max.x, texel_max.y, mip_level));
	float footprint_depth = min(min(depth00, depth10), min(depth01, depth11));

	return nearest_depth < footprint_depth;
}

#endif
