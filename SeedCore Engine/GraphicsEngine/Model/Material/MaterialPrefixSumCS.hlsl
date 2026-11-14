#include <GraphicsEngine/Shader/RootSignature.hlsli>

#define MATERIAL_SORT_BUCKET_COUNT 1024

struct MaterialPrefixSumBindings
{
	uint material_buckets_;
};
ConstantBuffer<MaterialPrefixSumBindings> bindings : register(b0, space1);

groupshared uint scan_data[MATERIAL_SORT_BUCKET_COUNT];

/**
* [EN]
* Material sort, pass 2/3: a single-group Hillis-Steele exclusive prefix sum
* over the bucket counts, turning each count into the first slot its bucket
* owns in Material.Pixels. MATERIAL_SORT_BUCKET_COUNT equals the largest
* thread group, so one dispatch covers it. The total of all buckets goes to
* the element after them, which bounds Material.Resolve.
*
* ---------------------------------------------------------------------
*
* [JP]
* マテリアルソート、パス 2/3。バケットの数に対する 1 グループの Hillis-Steele
* 排他的接頭和で、各数を Material.Pixels の中でそのバケットが持つ先頭の
* 位置に変える。MATERIAL_SORT_BUCKET_COUNT はスレッドグループの最大の大きさと
* 同じなので 1 回のディスパッチで済む。全バケットの合計はその後ろの要素に書き、
* Material.Resolve の範囲になる。
*/
[numthreads(MATERIAL_SORT_BUCKET_COUNT, 1, 1)]
void main(uint thread_id : SV_GroupThreadID)
{
	RWStructuredBuffer<uint> buckets = ResourceDescriptorHeap[bindings.material_buckets_];

	uint count = buckets[thread_id];
	scan_data[thread_id] = count;
	GroupMemoryBarrierWithGroupSync();

	[unroll]
	for (uint offset = 1; offset < MATERIAL_SORT_BUCKET_COUNT; offset *= 2)
	{
		uint added = 0;
		if (thread_id >= offset)
		{
			added = scan_data[thread_id - offset];
		}
		GroupMemoryBarrierWithGroupSync();
		scan_data[thread_id] += added;
		GroupMemoryBarrierWithGroupSync();
	}

	buckets[thread_id] = scan_data[thread_id] - count;
	if (thread_id == MATERIAL_SORT_BUCKET_COUNT - 1)
	{
		buckets[MATERIAL_SORT_BUCKET_COUNT] = scan_data[thread_id];
	}
}
