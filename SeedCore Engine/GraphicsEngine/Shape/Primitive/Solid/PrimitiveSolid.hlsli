#ifndef __PRIMITIVE_SOLID_HLSLI__
#define __PRIMITIVE_SOLID_HLSLI__

#include <GraphicsEngine/Shape/Primitive/Primitive.hlsli>
#include <GraphicsEngine/Model/Model.hlsli>

/// [EN] Bit of PrimitiveSolidStructuredBuffer::flags_ set while the row holds a shape drawn this frame.
#define PRIMITIVE_SOLID_ACTIVE 1

/**
* [EN]
* One solid shape: its world matrices (the unit mesh already scaled to the
* shape's size), which unit mesh it uses and that mesh's meshlet range,
* its texture coordinate transform, and the index buffer of its mesh for
* ray hits. Mirrors the C++ PrimitiveSolidStructuredBuffer (176 bytes).
*/
struct PrimitiveSolidStructuredBuffer
{
	row_major float4x4 world_;
	row_major float4x4 previous_world_;
	uint slot_;
	uint flags_;
	uint meshlet_offset_;
	uint meshlet_count_;
	float2 texcoord_min_;
	float2 texcoord_extent_;
	uint index_buffer_index_;
	uint3 primitive_solid_structured_buffer_padding_0_;
};

/**
* [EN]
* One changed solid row and the slot it goes to.
*/
struct PrimitiveSolidUpdate
{
	uint index_;
	uint3 primitive_solid_update_padding_0_;
	PrimitiveSolidStructuredBuffer data_;
};

/**
* [EN]
* One changed material row and the slot it goes to.
*/
struct PrimitiveMaterialUpdate
{
	uint index_;
	uint3 primitive_material_update_padding_0_;
	ModelMaterialStructuredBuffer data_;
};

/**
* [EN]
* Per-frame values of the expansion: how many solid rows are in use, the
* sections of Geometry's shared instance list, and the bindless indices of
* the unit meshes' buffers every shape shares.
*/
struct PrimitiveSolidConstantBuffer
{
	uint solid_count_;
	uint transparent_base_;
	uint capacity_;
	uint vertex_buffer_index_;
	uint meshlet_buffer_index_;
	uint meshlet_bound_buffer_index_;
	uint vertex_indices_buffer_index_;
	uint primitive_indices_buffer_index_;
	uint4 primitive_solid_constant_buffer_padding_0_;
};

#endif // __PRIMITIVE_SOLID_HLSLI__
