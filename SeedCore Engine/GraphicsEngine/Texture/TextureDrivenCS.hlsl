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
	uint texture_draw_list_;
};
ConstantBuffer<TextureExpandBindings> expand_bindings : register(b0, space1);

/**
* [EN]
* Texture.Update: writes the rows the CPU found changed into Texture.Units
* (dispatch constant 0) or Texture.Quads (1); one thread per row.
*
* ---------------------------------------------------------------------
*
* [JP]
* Texture.Update。CPU が変わったと判定した行を、Texture.Units（ディスパッチの
* 定数が 0）か Texture.Quads（1）へ書く。1 スレッドが 1 行。
*/
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

/**
* [EN]
* Texture.Expand: one thread per quad slot. A quad of a drawn unit writes its
* own index at Texture.DrawList[draw_base_ + its position inside the unit].
* The CPU chose every draw_base_ from the units' order, so the list comes out
* in drawing order without sorting or atomics. A slot outside its unit's
* current range is stale (the unit moved or shrank) and is skipped.
*
* ---------------------------------------------------------------------
*
* [JP]
* Texture.Expand。四角形の枠 1 つにつき 1 スレッド。描くユニットの四角形は、
* 自分の番号を Texture.DrawList[draw_base_ + ユニット内の位置] に書く。
* draw_base_ は CPU がユニットの順番から決めているので、ソートも atomic も
* 使わずに描く順番の並びになる。ユニットの今の範囲の外にある枠は古い
* （ユニットが移ったか縮んだ）ので飛ばす。
*/
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

	RWStructuredBuffer<uint> draw_list = ResourceDescriptorHeap[expand_bindings.texture_draw_list_];
	draw_list[unit.draw_base_ + local] = id.x;
}
