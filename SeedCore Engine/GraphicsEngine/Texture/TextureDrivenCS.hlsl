#include <GraphicsEngine/Texture/Texture.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct TextureUpdateBindings
{
	uint texture_unit_updates_;
	uint texture_quad_updates_;
	uint texture_units_;
	uint texture_quads_;
};
ConstantBuffer<TextureUpdateBindings> update_bindings : register(b0, space1);

struct TextureExpandBindings
{
	uint texture_units_;
	uint texture_quads_;
	uint texture_constants_;
	uint texture_draws_;
};
ConstantBuffer<TextureExpandBindings> expand_bindings : register(b0, space1);

[numthreads(64, 1, 1)]
void Update(uint3 id : SV_DispatchThreadID)
{
	uint update_count;
	uint update_stride;
	if (dispatch_bindings.constants_ == 0)
	{
		StructuredBuffer<TextureUnitUpdate> updates = ResourceDescriptorHeap[update_bindings.texture_unit_updates_];
		updates.GetDimensions(update_count, update_stride);
		if (id.x >= update_count)
		{
			return;
		}
		RWStructuredBuffer<TextureUnitStructuredBuffer> units = ResourceDescriptorHeap[update_bindings.texture_units_];
		units[updates[id.x].index_] = updates[id.x].data_;
	}
	else
	{
		StructuredBuffer<TextureQuadUpdate> updates = ResourceDescriptorHeap[update_bindings.texture_quad_updates_];
		updates.GetDimensions(update_count, update_stride);
		if (id.x >= update_count)
		{
			return;
		}
		RWStructuredBuffer<TextureQuadStructuredBuffer> quads = ResourceDescriptorHeap[update_bindings.texture_quads_];
		quads[updates[id.x].index_] = updates[id.x].data_;
	}
}

[numthreads(64, 1, 1)]
void Expand(uint3 id : SV_DispatchThreadID)
{
	ConstantBuffer<TextureConstantBuffer> constants = ResourceDescriptorHeap[expand_bindings.texture_constants_];
	if (id.x >= constants.quad_count_)
	{
		return;
	}

	StructuredBuffer<TextureQuadStructuredBuffer> quads = ResourceDescriptorHeap[expand_bindings.texture_quads_];
	StructuredBuffer<TextureUnitStructuredBuffer> units = ResourceDescriptorHeap[expand_bindings.texture_units_];
	uint unit_count;
	uint unit_stride;
	units.GetDimensions(unit_count, unit_stride);
	uint unit_index = quads[id.x].unit_;
	if (unit_index >= unit_count)
	{
		return;
	}
	TextureUnitStructuredBuffer unit = units[unit_index];
	uint local = id.x - unit.quad_offset_;
	if ((unit.flags_ & TEXTURE_UNIT_DRAWN) == 0 || id.x < unit.quad_offset_ || local >= unit.quad_count_)
	{
		return;
	}

	RWStructuredBuffer<uint> draws = ResourceDescriptorHeap[expand_bindings.texture_draws_];
	draws[unit.draw_base_ + local] = id.x;
}
