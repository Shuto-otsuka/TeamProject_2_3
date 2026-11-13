#include <GraphicsEngine/Raytracing/BottomLevel.hlsli>
#include <GraphicsEngine/Raytracing/TopLevel.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct TopLevelInstanceDesc
{
	float4 transform_[3];
	uint instance_id_mask_;
	uint contribution_flags_;
	uint2 bottom_level_;
};

struct TopLevelUpdateBindings
{
	uint top_level_counts_;
};
ConstantBuffer<TopLevelUpdateBindings> update_bindings : register(b0, space1);

struct TopLevelResolveBindings
{
	uint top_level_instances_;
	uint top_level_counts_;
	uint bottom_level_table_;
	uint top_level_descs_;
	uint top_level_hits_;
};
ConstantBuffer<TopLevelResolveBindings> resolve_bindings : register(b0, space1);

[numthreads(1, 1, 1)]
void Update(uint3 id : SV_DispatchThreadID)
{
	RWStructuredBuffer<uint> counts = ResourceDescriptorHeap[update_bindings.top_level_counts_];
	counts[0] = 0;
}

[numthreads(64, 1, 1)]
void Resolve(uint3 id : SV_DispatchThreadID)
{
	if (id.x >= dispatch_bindings.constants_)
	{
		return;
	}

	StructuredBuffer<uint> counts = ResourceDescriptorHeap[resolve_bindings.top_level_counts_];
	RWStructuredBuffer<TopLevelInstanceDesc> descs = ResourceDescriptorHeap[resolve_bindings.top_level_descs_];

	TopLevelInstanceDesc desc = (TopLevelInstanceDesc)0;
	if (id.x < counts[0])
	{
		StructuredBuffer<TopLevelInstanceStructuredBuffer> instances = ResourceDescriptorHeap[resolve_bindings.top_level_instances_];
		StructuredBuffer<BottomLevelTableStructuredBuffer> table = ResourceDescriptorHeap[resolve_bindings.bottom_level_table_];
		RWStructuredBuffer<TopLevelHitStructuredBuffer> hits = ResourceDescriptorHeap[resolve_bindings.top_level_hits_];

		TopLevelInstanceStructuredBuffer instance = instances[id.x];
		uint2 address = any(instance.bottom_level_ != 0) ? BottomLevelFind(table, instance.bottom_level_).address_ : uint2(0, 0);
		bool active = any(address != 0) && all(isfinite(instance.transform_[0])) && all(isfinite(instance.transform_[1])) && all(isfinite(instance.transform_[2]));

		desc.transform_ = instance.transform_;
		desc.instance_id_mask_ = id.x | ((active ? instance.instance_mask_ : 0) << 24);
		desc.contribution_flags_ = (active ? instance.flags_ : 0) << 24;
		desc.bottom_level_ = active ? address : uint2(0, 0);
		hits[id.x] = instance.hit_;
	}
	descs[id.x] = desc;
}
