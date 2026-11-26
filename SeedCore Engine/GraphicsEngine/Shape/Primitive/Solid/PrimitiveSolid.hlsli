#ifndef __PRIMITIVE_SOLID_HLSLI__
#define __PRIMITIVE_SOLID_HLSLI__

#include <GraphicsEngine/Shape/Primitive/Primitive.hlsli>
#include <GraphicsEngine/Model/Model.hlsli>

#define PRIMITIVE_SOLID_ACTIVE 1

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

struct PrimitiveSolidUpdate
{
	uint index_;
	uint3 primitive_solid_update_padding_0_;
	PrimitiveSolidStructuredBuffer data_;
};

struct PrimitiveMaterialUpdate
{
	uint index_;
	uint3 primitive_material_update_padding_0_;
	ModelMaterialStructuredBuffer data_;
};

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

#endif
