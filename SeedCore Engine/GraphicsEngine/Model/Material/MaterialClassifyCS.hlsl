#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>

#define MATERIAL_SORT_BUCKET_COUNT 1024

struct MaterialClassifyBindings
{
	uint geometry_depth_;
	uint geometry_visibility_id_;
	uint material_buckets_;
	uint material_velocity_;
	uint material_radiance_;
};
ConstantBuffer<MaterialClassifyBindings> bindings : register(b0, space1);

/**
* [EN]
* First dispatch of Material.Classify: clears every bucket count and the
* total that follows them.
*
* ---------------------------------------------------------------------
*
* [JP]
* Material.Classify の最初のディスパッチ。全バケットの数と、その後ろの総数を
* 0 にする。
*/
[numthreads(64, 1, 1)]
void Prepare(uint3 id : SV_DispatchThreadID)
{
	RWStructuredBuffer<uint> buckets = ResourceDescriptorHeap[bindings.material_buckets_];
	if (id.x <= MATERIAL_SORT_BUCKET_COUNT)
	{
		buckets[id.x] = 0;
	}
}

/**
* [EN]
* Material sort, pass 1/3. A foreground pixel counts itself into the bucket
* (instance_index % MATERIAL_SORT_BUCKET_COUNT); Material.PrefixSum turns the
* counts into offsets and Material.Scatter lays the pixels out bucket by
* bucket, so neighbouring threads of Material.Resolve share instances and
* textures. A background pixel instead gets its final values here: no
* emitted radiance, and the velocity of a point at a pseudo-infinite distance
* along its view ray, which isolates the camera rotation.
*
* ---------------------------------------------------------------------
*
* [JP]
* マテリアルソート、パス 1/3。前景のピクセルは自分をバケット
* (instance_index % MATERIAL_SORT_BUCKET_COUNT) に数える。Material.PrefixSum が
* 数をオフセットへ変え、Material.Scatter がバケット順にピクセルを並べるので、
* Material.Resolve の隣り合うスレッドがインスタンスとテクスチャを共有する。
* 背景のピクセルはここで最終の値を書く。放射は 0、速度は視線の方向へ
* 実質無限遠まで進めた点の速度で、カメラの回転だけを取り出す。
*/
[numthreads(8, 8, 1)]
void Classify(uint3 id : SV_DispatchThreadID)
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
	float depth = depth_texture.Load(int3(pixel, 0));
	Texture2D<uint2> visibility_id = ResourceDescriptorHeap[bindings.geometry_visibility_id_];
	if (depth == 0.0 || visibility_id.Load(int3(pixel, 0)).x == 0)
	{
		SceneConstantBuffer scene = GetSceneConstantBuffer();
		float2 uv = (float2(pixel) + 0.5) / float2(width, height);
		float4 world = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), scene.inverse_view_projection_);
		float3 view_direction = normalize(world.xyz / world.w - scene.camera_position_.xyz);
		float3 far_position = scene.camera_position_.xyz + view_direction * 100000.0;
		float4 current_clip = mul(float4(far_position, 1.0), scene.current_view_projection_);
		float4 previous_clip = mul(float4(far_position, 1.0), scene.previous_view_projection_);

		RWTexture2D<float2> velocity = ResourceDescriptorHeap[bindings.material_velocity_];
		RWTexture2D<float4> surface_radiance = ResourceDescriptorHeap[bindings.material_radiance_];
		velocity[pixel] = (current_clip.xy / current_clip.w - previous_clip.xy / previous_clip.w) * 0.5;
		surface_radiance[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	uint instance_index;
	uint meshlet_index;
	uint triangle_in_meshlet_index;
	UnpackVisibilityID(visibility_id.Load(int3(pixel, 0)), instance_index, meshlet_index, triangle_in_meshlet_index);

	RWStructuredBuffer<uint> buckets = ResourceDescriptorHeap[bindings.material_buckets_];
	InterlockedAdd(buckets[instance_index % MATERIAL_SORT_BUCKET_COUNT], 1);
}
