#include <GraphicsEngine/Shape/Primitive/Solid/PrimitiveSolid.hlsli>
#include <GraphicsEngine/Raytracing/TopLevel.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct PrimitiveSolidUpdateBindings
{
	uint primitive_solid_updates_;
	uint primitive_material_updates_;
	uint primitive_solids_;
	uint primitive_materials_;
};
ConstantBuffer<PrimitiveSolidUpdateBindings> update_bindings : register(b0, space1);

struct PrimitiveSolidExpandBindings
{
	uint primitive_solids_;
	uint primitive_materials_;
	uint primitive_constants_;
	uint geometry_instances_;
	uint geometry_counts_;
};
ConstantBuffer<PrimitiveSolidExpandBindings> expand_bindings : register(b0, space1);

struct PrimitiveSolidRaytracingBindings
{
	uint primitive_solids_;
	uint primitive_materials_;
	uint primitive_constants_;
	uint top_level_instances_;
	uint top_level_counts_;
};
ConstantBuffer<PrimitiveSolidRaytracingBindings> raytracing_bindings : register(b0, space1);

[numthreads(64, 1, 1)]
void Update(uint3 id : SV_DispatchThreadID)
{
	uint update_count;
	uint update_stride;
	if (dispatch_bindings.constants_ == 0)
	{
		StructuredBuffer<PrimitiveSolidUpdate> updates = ResourceDescriptorHeap[update_bindings.primitive_solid_updates_];
		updates.GetDimensions(update_count, update_stride);
		if (id.x >= update_count)
		{
			return;
		}
		RWStructuredBuffer<PrimitiveSolidStructuredBuffer> solids = ResourceDescriptorHeap[update_bindings.primitive_solids_];
		solids[updates[id.x].index_] = updates[id.x].data_;
	}
	else
	{
		StructuredBuffer<PrimitiveMaterialUpdate> updates = ResourceDescriptorHeap[update_bindings.primitive_material_updates_];
		updates.GetDimensions(update_count, update_stride);
		if (id.x >= update_count)
		{
			return;
		}
		RWStructuredBuffer<ModelMaterialStructuredBuffer> materials = ResourceDescriptorHeap[update_bindings.primitive_materials_];
		materials[updates[id.x].index_] = updates[id.x].data_;
	}
}

[numthreads(64, 1, 1)]
void Expand(uint3 id : SV_DispatchThreadID)
{
	ConstantBuffer<PrimitiveSolidConstantBuffer> constants = ResourceDescriptorHeap[expand_bindings.primitive_constants_];
	if (id.x >= constants.solid_count_)
	{
		return;
	}

	StructuredBuffer<PrimitiveSolidStructuredBuffer> solids = ResourceDescriptorHeap[expand_bindings.primitive_solids_];
	PrimitiveSolidStructuredBuffer solid = solids[id.x];
	if ((solid.flags_ & PRIMITIVE_SOLID_ACTIVE) == 0 || solid.meshlet_count_ == 0)
	{
		return;
	}

	StructuredBuffer<ModelMaterialStructuredBuffer> materials = ResourceDescriptorHeap[expand_bindings.primitive_materials_];
	RWStructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[expand_bindings.geometry_instances_];
	RWStructuredBuffer<uint> counts = ResourceDescriptorHeap[expand_bindings.geometry_counts_];
	ModelMaterialStructuredBuffer material = materials[id.x];

	ModelStructuredBuffer instance = (ModelStructuredBuffer)0;
	instance.shading_ = material.shading_;
	instance.texture_ = material.texture_;
	instance.extension_ = material.extension_;

	instance.geometry_.vertex_buffer_index_ = constants.vertex_buffer_index_;
	instance.geometry_.meshlet_buffer_index_ = constants.meshlet_buffer_index_;
	instance.geometry_.meshlet_bound_buffer_index_ = constants.meshlet_bound_buffer_index_;
	instance.geometry_.vertex_indices_buffer_index_ = constants.vertex_indices_buffer_index_;
	instance.geometry_.primitive_indices_buffer_index_ = constants.primitive_indices_buffer_index_;

	instance.streaming_.position_min_ = float3(-1.0, -1.0, -1.0);
	instance.streaming_.position_extent_ = float3(2.0, 2.0, 2.0);
	instance.streaming_.texcoord_min_u_ = solid.texcoord_min_.x;
	instance.streaming_.texcoord_min_v_ = solid.texcoord_min_.y;
	instance.streaming_.texcoord_extent_ = solid.texcoord_extent_;
	instance.streaming_.lod_error_ = 0.0;
	instance.streaming_.lod_level_ = 0;

	instance.skining_.skin_index_ = MODEL_INVALID;
	instance.skining_.skin_vertex_buffer_index_ = MODEL_INVALID;
	instance.morph_.morph_delta_buffer_index_ = MODEL_INVALID;
	instance.morph_.vertex_morph_source_buffer_index_ = MODEL_INVALID;

	float3x3 world_3x3 = (float3x3)solid.world_;
	float3 cofactor0 = cross(world_3x3[1], world_3x3[2]);
	float3 cofactor1 = cross(world_3x3[2], world_3x3[0]);
	float3 cofactor2 = cross(world_3x3[0], world_3x3[1]);
	float determinant = dot(world_3x3[0], cofactor0);
	float inverse_determinant = abs(determinant) > 1e-20 ? 1.0 / determinant : 0.0;
	float3x3 inverse_transpose = float3x3(cofactor0, cofactor1, cofactor2) * inverse_determinant;
	float3 inverse_translation = -mul(solid.world_[3].xyz, transpose(inverse_transpose));
	instance.transform_.world_ = solid.world_;
	instance.transform_.inverse_transpose_world_ = float4x4(float4(inverse_transpose[0], inverse_translation.x), float4(inverse_transpose[1], inverse_translation.y), float4(inverse_transpose[2], inverse_translation.z), float4(0.0, 0.0, 0.0, 1.0));
	instance.transform_.previous_world_ = solid.previous_world_;

	uint chunk_count = (solid.meshlet_count_ + MODEL_MESHLETS_PER_INSTANCE - 1) / MODEL_MESHLETS_PER_INSTANCE;
	bool transparent = material.shading_.blend_ != 0;
	uint counter = transparent ? MODEL_COUNT_TRANSPARENT : MODEL_COUNT_OPAQUE;
	uint section_base = transparent ? constants.transparent_base_ : 0;
	uint section_end = transparent ? constants.capacity_ : constants.transparent_base_;

	uint first;
	InterlockedAdd(counts[counter], chunk_count, first);
	if (section_base + first + chunk_count > section_end)
	{
		InterlockedAdd(counts[MODEL_COUNT_OVERFLOW], chunk_count);
		return;
	}

	for (uint chunk_index = 0; chunk_index < chunk_count; chunk_index++)
	{
		uint meshlet_offset = chunk_index * MODEL_MESHLETS_PER_INSTANCE;
		instance.geometry_.meshlet_offset_ = solid.meshlet_offset_ + meshlet_offset;
		instance.geometry_.meshlet_count_ = min(MODEL_MESHLETS_PER_INSTANCE, solid.meshlet_count_ - meshlet_offset);
		instances[section_base + first + chunk_index] = instance;
	}
}

[numthreads(64, 1, 1)]
void Raytracing(uint3 id : SV_DispatchThreadID)
{
	if (id.x >= dispatch_bindings.constants_)
	{
		return;
	}

	StructuredBuffer<PrimitiveSolidStructuredBuffer> solids = ResourceDescriptorHeap[raytracing_bindings.primitive_solids_];
	PrimitiveSolidStructuredBuffer solid = solids[id.x];
	if ((solid.flags_ & PRIMITIVE_SOLID_ACTIVE) == 0)
	{
		return;
	}

	ConstantBuffer<PrimitiveSolidConstantBuffer> constants = ResourceDescriptorHeap[raytracing_bindings.primitive_constants_];
	RWStructuredBuffer<TopLevelInstanceStructuredBuffer> instances = ResourceDescriptorHeap[raytracing_bindings.top_level_instances_];
	RWStructuredBuffer<uint> counts = ResourceDescriptorHeap[raytracing_bindings.top_level_counts_];
	uint capacity;
	uint stride;
	instances.GetDimensions(capacity, stride);

	uint slot;
	InterlockedAdd(counts[0], 1, slot);
	if (slot >= capacity)
	{
		return;
	}

	float4x4 world = solid.world_;
	TopLevelInstanceStructuredBuffer instance;
	instance.transform_[0] = float4(world[0][0], world[1][0], world[2][0], world[3][0]);
	instance.transform_[1] = float4(world[0][1], world[1][1], world[2][1], world[3][1]);
	instance.transform_[2] = float4(world[0][2], world[1][2], world[2][2], world[3][2]);
	instance.bottom_level_ = uint2(solid.slot_, 3u << 24);
	instance.instance_mask_ = 0xFF;
	StructuredBuffer<ModelMaterialStructuredBuffer> materials = ResourceDescriptorHeap[raytracing_bindings.primitive_materials_];
	instance.flags_ = materials[id.x].shading_.blend_ != 0 ? 0x8 : 0;
	instance.hit_.vertex_buffer_index_ = constants.vertex_buffer_index_;
	instance.hit_.index_buffer_index_ = solid.index_buffer_index_;
	instance.hit_.surface_buffer_index_ = MODEL_INVALID;
	instance.hit_.material_buffer_index_ = raytracing_bindings.primitive_materials_;
	instance.hit_.material_offset_ = id.x;
	instance.hit_.top_level_hit_structured_buffer_padding_0_ = 0;
	instance.hit_.texcoord_min_ = solid.texcoord_min_;
	instance.hit_.texcoord_extent_ = solid.texcoord_extent_;
	instance.hit_.top_level_hit_structured_buffer_padding_1_ = 0;
	instance.hit_.top_level_hit_structured_buffer_padding_2_ = 0;
	instances[slot] = instance;
}
