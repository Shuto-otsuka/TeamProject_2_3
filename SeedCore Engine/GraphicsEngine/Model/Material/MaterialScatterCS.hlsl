#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

#define MATERIAL_SORT_BUCKET_COUNT 1024

struct MaterialScatterBindings
{
	uint geometry_depth_;
	uint geometry_visibility_id_;
	uint material_buckets_;
	uint material_pixels_;
};
ConstantBuffer<MaterialScatterBindings> bindings : register(b0, space1);

/**
* [EN]
* Material sort, pass 3/3: every foreground pixel claims the next slot of its
* bucket (the offsets from Material.PrefixSum become running write cursors
* here) and stores its linear pixel position there.
*
* ---------------------------------------------------------------------
*
* [JP]
* マテリアルソート、パス 3/3。前景のピクセルは自分のバケットの次の位置を確保し
* (Material.PrefixSum のオフセットがここで書き込み位置に変わる)、そこへ自分の
* 線形のピクセル位置を書く。
*/
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	Texture2D<float> depth_texture = ResourceDescriptorHeap[bindings.geometry_depth_];
	uint width;
	uint height;
	depth_texture.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	uint2 pixel = id.xy;
	Texture2D<uint2> visibility_id = ResourceDescriptorHeap[bindings.geometry_visibility_id_];
	if (depth_texture.Load(int3(pixel, 0)) == 0.0 || visibility_id.Load(int3(pixel, 0)).x == 0)
	{
		return;
	}

	uint instance_index;
	uint meshlet_index;
	uint triangle_in_meshlet_index;
	UnpackVisibilityID(visibility_id.Load(int3(pixel, 0)), instance_index, meshlet_index, triangle_in_meshlet_index);

	RWStructuredBuffer<uint> buckets = ResourceDescriptorHeap[bindings.material_buckets_];
	uint slot;
	InterlockedAdd(buckets[instance_index % MATERIAL_SORT_BUCKET_COUNT], 1, slot);

	RWStructuredBuffer<uint> pixels = ResourceDescriptorHeap[bindings.material_pixels_];
	pixels[slot] = pixel.y * width + pixel.x;
}
