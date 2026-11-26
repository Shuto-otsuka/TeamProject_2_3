#include <GraphicsEngine/Shader/RootSignature.hlsli>

#define MATERIAL_SORT_BUCKET_COUNT 1024

struct MaterialPrefixSumBindings
{
	uint material_buckets_;
};
ConstantBuffer<MaterialPrefixSumBindings> bindings : register(b0, space1);

groupshared uint scan_data[MATERIAL_SORT_BUCKET_COUNT];

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
