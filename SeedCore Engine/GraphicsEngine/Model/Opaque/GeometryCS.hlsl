#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct GeometryPyramidBindings
{
	uint geometry_depth_;
	uint geometry_hierarchical_depth_;
};
ConstantBuffer<GeometryPyramidBindings> pyramid_bindings : register(b0, space1);

struct GeometryUpdateBindings
{
	uint geometry_counts_;
};
ConstantBuffer<GeometryUpdateBindings> update_bindings : register(b0, space1);

/**
* [EN]
* Starts the frame's shared instance list: every section counter (opaque,
* transparent, fur, overflow) goes back to zero, and each source then appends
* its instances behind it.
*/
[numthreads(4, 1, 1)]
void Update(uint3 id : SV_DispatchThreadID)
{
	RWStructuredBuffer<uint> counts = ResourceDescriptorHeap[update_bindings.geometry_counts_];
	counts[id.x] = 0;
}

/**
* [EN]
* Builds one mip of the Hi-Z pyramid per dispatch; the dispatch constant is
* the destination mip. Each thread writes one texel as the MINIMUM of its 2x2
* source footprint, which under reversed-Z is the farthest surface, as the
* conservative occlusion test needs. Mip 0 (half the render resolution)
* reduces Depth; every later mip reduces the previous one through its UAV
* (per-mip UAVs are contiguous from geometry_hierarchical_depth_). Odd source sizes
* fold the extra edge row/column in, so no depth sample is dropped.
*/
[numthreads(8, 8, 1)]
void Pyramid(uint3 id : SV_DispatchThreadID)
{
	uint mip = dispatch_bindings.constants_;
	RWTexture2D<float> destination = ResourceDescriptorHeap[pyramid_bindings.geometry_hierarchical_depth_ + mip];
	uint destination_width;
	uint destination_height;
	destination.GetDimensions(destination_width, destination_height);
	if (id.x >= destination_width || id.y >= destination_height)
	{
		return;
	}

	Texture2D<float> depth = ResourceDescriptorHeap[pyramid_bindings.geometry_depth_];
	RWTexture2D<float> source = ResourceDescriptorHeap[pyramid_bindings.geometry_hierarchical_depth_ + max(mip, 1) - 1];
	uint source_width;
	uint source_height;
	if (mip == 0)
	{
		depth.GetDimensions(source_width, source_height);
	}
	else
	{
		source.GetDimensions(source_width, source_height);
	}

	uint2 base = id.xy * 2;
	uint footprint_x = (source_width & 1) != 0 && id.x == destination_width - 1 ? 3 : 2;
	uint footprint_y = (source_height & 1) != 0 && id.y == destination_height - 1 ? 3 : 2;
	float result = 1.0;
	for (uint offset_y = 0; offset_y < footprint_y; offset_y++)
	{
		for (uint offset_x = 0; offset_x < footprint_x; offset_x++)
		{
			uint2 position = min(base + uint2(offset_x, offset_y), uint2(source_width - 1, source_height - 1));
			result = min(result, mip == 0 ? depth.Load(int3(position, 0)) : source[position]);
		}
	}
	destination[id.xy] = result;
}
