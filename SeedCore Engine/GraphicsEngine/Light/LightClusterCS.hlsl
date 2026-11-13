#include <GraphicsEngine/Light/Light.hlsli>
#include <GraphicsEngine/Light/LightCluster.h>
#include <GraphicsEngine/Shader/Scene.hlsli>

struct LightBuildBindings
{
	uint light_list_;
	uint light_constants_;
	uint light_cluster_grid_;
};
ConstantBuffer<LightBuildBindings> bindings : register(b0, space1);

/**
* [EN]
* One thread per cluster of the view: tiles of LIGHT_CLUSTER_TILE_SIZE pixels
* by LIGHT_CLUSTER_DEPTH_SLICES slices spaced logarithmically between the near
* and far planes. The thread builds its cluster's view-space box from the
* tile's corner rays at the slice's two depths, tests every light's range
* sphere against it, and writes the count followed by the first
* LIGHT_CLUSTER_MAX_LIGHTS hits straight into its own slot. Each cluster is
* written by exactly one thread, so nothing is cleared beforehand and the
* order of the indices is the same every frame.
*
* ---------------------------------------------------------------------
*
* [JP]
* ビューのクラスタ 1 つにつき 1 スレッド。LIGHT_CLUSTER_TILE_SIZE ピクセルの
* タイルと、近平面と遠平面の間を対数で分けた LIGHT_CLUSTER_DEPTH_SLICES 個の
* 深さ。スレッドはタイルの角の視線と深さの 2 つの値から自分のクラスタの
* ビュー空間の箱を作り、全ライトの範囲の球と当たりを調べて、数と先頭
* LIGHT_CLUSTER_MAX_LIGHTS 件の番号を自分の欄へ直接書く。各クラスタを書くのは
* 1 スレッドだけなので前のクリアが要らず、番号の並びは毎フレーム同じになる。
*/
[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	SceneConstantBuffer scene = GetSceneConstantBuffer();
	uint2 tile_count = (uint2(scene.screen_size_) + LIGHT_CLUSTER_TILE_SIZE - 1) / LIGHT_CLUSTER_TILE_SIZE;
	if (id.x >= tile_count.x * tile_count.y * LIGHT_CLUSTER_DEPTH_SLICES)
	{
		return;
	}

	uint tile_x = id.x % tile_count.x;
	uint tile_y = (id.x / tile_count.x) % tile_count.y;
	uint slice = id.x / (tile_count.x * tile_count.y);

	float log_near = log(scene.near_plane_);
	float log_range = log(scene.far_plane_) - log_near;
	float slice_near = exp(log_near + log_range * (float(slice) / float(LIGHT_CLUSTER_DEPTH_SLICES)));
	float slice_far = exp(log_near + log_range * (float(slice + 1) / float(LIGHT_CLUSTER_DEPTH_SLICES)));

	float2 pixel_min = float2(tile_x, tile_y) * LIGHT_CLUSTER_TILE_SIZE;
	float2 pixel_max = min(pixel_min + LIGHT_CLUSTER_TILE_SIZE, scene.screen_size_);
	float3 box_min = float3(1e30, 1e30, 1e30);
	float3 box_max = float3(-1e30, -1e30, -1e30);
	for (uint corner = 0; corner < 4; corner++)
	{
		float2 pixel = float2((corner & 1) != 0 ? pixel_max.x : pixel_min.x, (corner & 2) != 0 ? pixel_max.y : pixel_min.y);
		float2 ndc = float2(pixel.x / scene.screen_size_.x * 2.0 - 1.0, 1.0 - pixel.y / scene.screen_size_.y * 2.0);
		float4 view_point = mul(float4(ndc, 1.0, 1.0), scene.inverse_projection_);
		float3 ray = view_point.xyz / view_point.w;
		ray /= ray.z;
		box_min = min(box_min, min(ray * slice_near, ray * slice_far));
		box_max = max(box_max, max(ray * slice_near, ray * slice_far));
	}

	ConstantBuffer<LightConstantBuffer> constants = ResourceDescriptorHeap[bindings.light_constants_];
	RWStructuredBuffer<uint> grid = ResourceDescriptorHeap[bindings.light_cluster_grid_];
	uint base = id.x * (1 + LIGHT_CLUSTER_MAX_LIGHTS);
	uint count = 0;
	if (constants.light_count_ > 0)
	{
		StructuredBuffer<LightStructuredBuffer> lights = ResourceDescriptorHeap[bindings.light_list_];
		for (uint light_index = 0; light_index < constants.light_count_ && count < LIGHT_CLUSTER_MAX_LIGHTS; light_index++)
		{
			LightStructuredBuffer light = lights[light_index];
			float3 center = mul(float4(light.position_, 1.0), scene.view_).xyz;
			float3 closest = clamp(center, box_min, box_max);
			float3 offset = center - closest;
			if (dot(offset, offset) <= light.range_ * light.range_)
			{
				grid[base + 1 + count] = light_index;
				count++;
			}
		}
	}
	grid[base] = count;
}
