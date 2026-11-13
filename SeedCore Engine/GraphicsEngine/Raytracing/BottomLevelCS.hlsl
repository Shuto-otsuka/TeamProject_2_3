#include <GraphicsEngine/Raytracing/BottomLevel.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct BottomLevelTableUpdate
{
	uint slot_;
	uint3 bottom_level_table_update_padding_0_;
	BottomLevelTableStructuredBuffer data_;
};

struct BottomLevelUpdateBindings
{
	uint bottom_level_table_updates_;
	uint bottom_level_table_;
};
ConstantBuffer<BottomLevelUpdateBindings> update_bindings : register(b0, space1);

[numthreads(64, 1, 1)]
void Clear(uint3 id : SV_DispatchThreadID)
{
	if (id.x >= dispatch_bindings.constants_)
	{
		return;
	}

	RWStructuredBuffer<BottomLevelTableStructuredBuffer> table = ResourceDescriptorHeap[update_bindings.bottom_level_table_];
	BottomLevelTableStructuredBuffer row = (BottomLevelTableStructuredBuffer)0;
	row.vertex_offset_ = 0xFFFFFFFF;
	table[id.x] = row;
}

[numthreads(64, 1, 1)]
void Update(uint3 id : SV_DispatchThreadID)
{
	if (id.x >= dispatch_bindings.constants_)
	{
		return;
	}

	StructuredBuffer<BottomLevelTableUpdate> updates = ResourceDescriptorHeap[update_bindings.bottom_level_table_updates_];
	RWStructuredBuffer<BottomLevelTableStructuredBuffer> table = ResourceDescriptorHeap[update_bindings.bottom_level_table_];
	BottomLevelTableUpdate update = updates[id.x];
	table[update.slot_] = update.data_;
}
