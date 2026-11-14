#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Culling/FrustumCulling.hlsli>
#include <GraphicsEngine/Culling/BackfaceCulling.hlsli>
#include <GraphicsEngine/Culling/OcclusionCulling.hlsli>
#include <GraphicsEngine/Culling/Culling.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

struct CullingEarlyBindings
{
	uint geometry_instances_;
	uint geometry_counts_;
	uint geometry_hi_z_history_;
	uint culling_occluded_meshlets_;
	uint culling_opaque_meshlets_;
	uint culling_meshlet_arguments_;
	uint culling_transparent_arguments_;
	uint culling_opaque_chunks_;
	uint culling_occluded_chunks_;
	uint culling_chunk_arguments_;
	uint geometry_occluded_meshlets_;
};
ConstantBuffer<CullingEarlyBindings> early_bindings : register(b0, space1);

struct CullingLateBindings
{
	uint geometry_instances_;
	uint geometry_counts_;
	uint geometry_hi_z_;
	uint culling_occluded_meshlets_;
	uint culling_opaque_meshlets_;
	uint culling_meshlet_arguments_;
};
ConstantBuffer<CullingLateBindings> late_bindings : register(b0, space1);

struct CullingTransparentBindings
{
	uint geometry_instances_;
	uint geometry_counts_;
	uint geometry_hi_z_;
	uint culling_transparent_meshlets_;
	uint culling_transparent_chunks_;
	uint culling_transparent_arguments_;
};
ConstantBuffer<CullingTransparentBindings> transparent_bindings : register(b0, space1);

/**
* [EN]
* First dispatch of Culling.Culling.Early (one thread). Clears the counts and
* writes the fixed parts of the early, late and transparent indirect commands.
* The top bit of the dispatch constant selects the layout: DispatchMesh with
* 1024 groups per row (the row count grows as entries are appended), or the
* fallback DrawInstanced with 124 triangles' worth of vertices per meshlet
* instance. The mesh path also resets the chunk commands and the count at the
* head of Geometry.OccludedMeshlets.
*/
[numthreads(1, 1, 1)]
void Prepare(uint3 id : SV_DispatchThreadID)
{
	RWStructuredBuffer<uint> arguments = ResourceDescriptorHeap[early_bindings.culling_meshlet_arguments_];
	bool mesh = (dispatch_bindings.constants_ & 0x80000000) != 0;
	for (uint index = 0; index < 16; index++)
	{
		arguments[index] = 0;
	}
	for (uint command = CULLING_EARLY_ARGUMENTS; command <= CULLING_LATE_ARGUMENTS; command++)
	{
		if (mesh)
		{
			arguments[command * 3 + 0] = 1024;
			arguments[command * 3 + 2] = 1;
		}
		else
		{
			arguments[command * 4 + 0] = 124 * 3;
		}
	}

	RWStructuredBuffer<uint> transparent_arguments = ResourceDescriptorHeap[early_bindings.culling_transparent_arguments_];
	for (uint index = 0; index < 8; index++)
	{
		transparent_arguments[index] = 0;
	}
	if (mesh)
	{
		transparent_arguments[0] = 1024;
		transparent_arguments[2] = 1;
	}
	else
	{
		transparent_arguments[0] = 124 * 3;
	}

	if (mesh)
	{
		RWStructuredBuffer<uint> chunk_arguments = ResourceDescriptorHeap[early_bindings.culling_chunk_arguments_];
		for (uint index = 0; index < 16; index++)
		{
			chunk_arguments[index] = 0;
		}
		for (uint command = CULLING_EARLY_ARGUMENTS; command <= CULLING_LATE_ARGUMENTS; command++)
		{
			chunk_arguments[command * 3 + 0] = 1024;
			chunk_arguments[command * 3 + 2] = 1;
		}
		RWStructuredBuffer<uint> occluded_meshlets = ResourceDescriptorHeap[early_bindings.geometry_occluded_meshlets_];
		occluded_meshlets[0] = 0;
	}
}

/**
* [EN]
* Mesh path: one thread per chunk (instance) of the opaque section of
* ModelInstances; the chunk's whole quantization box is tested against the
* frustum, and a chunk hidden by the previous frame's Hi-Z goes to
* Culling.OccludedChunks for the late amplification pass instead of
* Culling.OpaqueChunks. The amplification shader then tests each meshlet.
* Fallback path: one thread per meshlet of the same section (the low bits of
* the dispatch constant are the section's end). Frustum and backface
* rejections are final; a meshlet that survives them but is hidden by the
* previous frame's Hi-Z goes to Culling.OccludedMeshlets so the late pass can test it
* again against this frame's depth (when Culling.OccludedMeshlets is full it is drawn
* early instead). Each visible meshlet widens the early command so it covers
* its slot. Skinned instances have rest-pose bounds, so they skip every test
* on both paths.
*/
[numthreads(64, 1, 1)]
void Early(uint3 id : SV_DispatchThreadID)
{
	uint section_end = dispatch_bindings.constants_ & 0x7FFFFFFF;
	bool mesh = (dispatch_bindings.constants_ & 0x80000000) != 0;
	uint instance_index = mesh ? id.x : id.x / MODEL_MESHLETS_PER_INSTANCE;
	uint meshlet_local = mesh ? 0 : id.x % MODEL_MESHLETS_PER_INSTANCE;

	StructuredBuffer<uint> counts = ResourceDescriptorHeap[early_bindings.geometry_counts_];
	if (instance_index >= min(counts[MODEL_COUNT_OPAQUE], section_end))
	{
		return;
	}

	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[early_bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[instance_index];
	if (meshlet_local >= instance.geometry_.meshlet_count_)
	{
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	uint show_flag = instance.shading_.actor_ == MODEL_INVALID ? SHOW_FLAG_PRIMITIVE : (instance.geometry_.layer_ != 0 ? SHOW_FLAG_FUR : SHOW_FLAG_MODEL);
	if ((scene.show_flags_low_ & show_flag) == 0)
	{
		return;
	}

	if (mesh)
	{
		bool hidden = false;
		if (instance.skining_.skin_index_ == MODEL_INVALID)
		{
			float chunk_scale = max(max(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz)), length(instance.transform_.world_[2].xyz));
			float3 chunk_center = mul(float4(instance.streaming_.position_min_ + instance.streaming_.position_extent_ * 0.5, 1.0), instance.transform_.world_).xyz;
			float chunk_radius = length(instance.streaming_.position_extent_) * 0.5 * chunk_scale;
			if (instance.geometry_.layer_ != 0)
			{
				chunk_radius += instance.shading_.fur_length_ * (chunk_scale + 0.35);
			}
			if (!IsInsideFrustum(chunk_center, chunk_radius, scene.current_view_projection_))
			{
				return;
			}
			hidden = IsOccluded(chunk_center, chunk_radius, scene.previous_view_projection_, scene.camera_position_.xyz, scene.projection_._m11, scene.screen_size_, early_bindings.geometry_hi_z_history_);
		}

		RWStructuredBuffer<uint> chunk_arguments = ResourceDescriptorHeap[early_bindings.culling_chunk_arguments_];
		uint chunk_slot;
		if (hidden)
		{
			RWStructuredBuffer<uint> occluded_chunks = ResourceDescriptorHeap[early_bindings.culling_occluded_chunks_];
			InterlockedAdd(chunk_arguments[CULLING_OCCLUDED_CHUNK_COUNT], 1, chunk_slot);
			occluded_chunks[chunk_slot] = instance_index;
			InterlockedMax(chunk_arguments[CULLING_LATE_ARGUMENTS * 3 + 1], chunk_slot / 1024 + 1);
		}
		else
		{
			RWStructuredBuffer<uint> opaque_chunks = ResourceDescriptorHeap[early_bindings.culling_opaque_chunks_];
			InterlockedAdd(chunk_arguments[CULLING_OPAQUE_CHUNK_COUNT], 1, chunk_slot);
			opaque_chunks[chunk_slot] = instance_index;
			InterlockedMax(chunk_arguments[CULLING_EARLY_ARGUMENTS * 3 + 1], chunk_slot / 1024 + 1);
		}
		return;
	}

	RWStructuredBuffer<uint> arguments = ResourceDescriptorHeap[early_bindings.culling_meshlet_arguments_];
	uint meshlet_index = instance.geometry_.meshlet_offset_ + meshlet_local;
	uint2 entry = uint2(instance_index, meshlet_index);

	if (instance.skining_.skin_index_ == MODEL_INVALID)
	{
		StructuredBuffer<ModelMeshletBound> bounds = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.meshlet_bound_buffer_index_)];
		ModelMeshletBound bound = bounds[meshlet_index];
		float world_scale = max(max(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz)), length(instance.transform_.world_[2].xyz));
		float3 center = mul(float4(bound.center_, 1.0), instance.transform_.world_).xyz;
		float radius = bound.radius_ * world_scale;
		if (instance.geometry_.layer_ != 0)
		{
			radius += instance.shading_.fur_length_ * (world_scale + 0.35);
		}

		if (!IsInsideFrustum(center, radius, scene.current_view_projection_))
		{
			return;
		}
		if (instance.shading_.double_sided_ == 0)
		{
			float3 cone_axis = normalize(mul(float4(bound.cone_axis_, 0.0), instance.transform_.world_).xyz);
			if (IsBackfacing(center, radius, cone_axis, bound.cone_cutoff_, scene.camera_position_.xyz))
			{
				return;
			}
		}
		if (IsOccluded(center, radius, scene.previous_view_projection_, scene.camera_position_.xyz, scene.projection_._m11, scene.screen_size_, early_bindings.geometry_hi_z_history_))
		{
			RWStructuredBuffer<uint2> occluded_meshlets = ResourceDescriptorHeap[early_bindings.culling_occluded_meshlets_];
			uint occluded_capacity;
			uint occluded_stride;
			occluded_meshlets.GetDimensions(occluded_capacity, occluded_stride);
			uint occluded_slot;
			InterlockedAdd(arguments[CULLING_OCCLUDED_COUNT], 1, occluded_slot);
			if (occluded_slot < occluded_capacity)
			{
				occluded_meshlets[occluded_slot] = entry;
				return;
			}
		}
	}

	RWStructuredBuffer<uint2> opaque_meshlets = ResourceDescriptorHeap[early_bindings.culling_opaque_meshlets_];
	uint capacity;
	uint stride;
	opaque_meshlets.GetDimensions(capacity, stride);
	uint slot;
	InterlockedAdd(arguments[CULLING_EARLY_COUNT], 1, slot);
	if (slot >= capacity)
	{
		InterlockedAdd(arguments[CULLING_OVERFLOW], 1);
		return;
	}
	opaque_meshlets[slot] = entry;
	InterlockedMax(arguments[CULLING_EARLY_ARGUMENTS * 4 + 1], slot + 1);
}

/**
* [EN]
* Fallback path only: one thread per entry of Culling.OccludedMeshlets. Tests the
* meshlet again against the Hi-Z built from this frame's early depth, and
* appends the ones now visible right after the early entries of the draw
* list, widening the late command the same way as Early. The mesh path makes
* this decision in the late amplification pass instead.
*/
[numthreads(64, 1, 1)]
void Late(uint3 id : SV_DispatchThreadID)
{
	RWStructuredBuffer<uint> arguments = ResourceDescriptorHeap[late_bindings.culling_meshlet_arguments_];
	StructuredBuffer<uint2> occluded_meshlets = ResourceDescriptorHeap[late_bindings.culling_occluded_meshlets_];
	uint occluded_capacity;
	uint occluded_stride;
	occluded_meshlets.GetDimensions(occluded_capacity, occluded_stride);
	if (id.x >= min(arguments[CULLING_OCCLUDED_COUNT], occluded_capacity))
	{
		return;
	}

	uint2 entry = occluded_meshlets[id.x];
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[late_bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[entry.x];
	StructuredBuffer<ModelMeshletBound> bounds = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.meshlet_bound_buffer_index_)];
	ModelMeshletBound bound = bounds[entry.y];
	float world_scale = max(max(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz)), length(instance.transform_.world_[2].xyz));
	float3 center = mul(float4(bound.center_, 1.0), instance.transform_.world_).xyz;
	float radius = bound.radius_ * world_scale;
	if (instance.geometry_.layer_ != 0)
	{
		radius += instance.shading_.fur_length_ * (world_scale + 0.35);
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	if (IsOccluded(center, radius, scene.current_view_projection_, scene.camera_position_.xyz, scene.projection_._m11, scene.screen_size_, late_bindings.geometry_hi_z_))
	{
		return;
	}

	RWStructuredBuffer<uint2> opaque_meshlets = ResourceDescriptorHeap[late_bindings.culling_opaque_meshlets_];
	uint capacity;
	uint stride;
	opaque_meshlets.GetDimensions(capacity, stride);
	uint base = min(arguments[CULLING_EARLY_COUNT], capacity);
	uint slot;
	InterlockedAdd(arguments[CULLING_LATE_COUNT], 1, slot);
	if (base + slot >= capacity)
	{
		InterlockedAdd(arguments[CULLING_OVERFLOW], 1);
		return;
	}
	opaque_meshlets[base + slot] = entry;
	InterlockedMax(arguments[CULLING_LATE_ARGUMENTS * 4 + 1], slot + 1);
}

/**
* [EN]
* Transparent instances are drawn once, after the opaque depth is complete,
* so they are tested against the frustum and the Hi-Z built from this frame's
* final depth in a single pass (the low bits of the dispatch constant are the
* section's start). Mesh path: one thread per chunk, tested by its whole
* quantization box; survivors are appended to Culling.TransparentChunks and
* the amplification shader tests each meshlet. Fallback path: one thread per
* meshlet, also tested by its normal cone, appended to
* Culling.TransparentMeshlets. Survivors widen the transparent command; skinned
* instances have rest-pose bounds and skip every test.
*/
[numthreads(64, 1, 1)]
void Transparent(uint3 id : SV_DispatchThreadID)
{
	uint section_base = dispatch_bindings.constants_ & 0x7FFFFFFF;
	bool mesh = (dispatch_bindings.constants_ & 0x80000000) != 0;
	uint instance_local = mesh ? id.x : id.x / MODEL_MESHLETS_PER_INSTANCE;
	uint meshlet_local = mesh ? 0 : id.x % MODEL_MESHLETS_PER_INSTANCE;

	StructuredBuffer<uint> counts = ResourceDescriptorHeap[transparent_bindings.geometry_counts_];
	if (instance_local >= counts[MODEL_COUNT_TRANSPARENT])
	{
		return;
	}

	uint instance_index = section_base + instance_local;
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[transparent_bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[instance_index];
	if (meshlet_local >= instance.geometry_.meshlet_count_)
	{
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	uint show_flag = instance.shading_.actor_ == MODEL_INVALID ? SHOW_FLAG_PRIMITIVE : (instance.geometry_.layer_ != 0 ? SHOW_FLAG_FUR : SHOW_FLAG_MODEL);
	if ((scene.show_flags_low_ & show_flag) == 0)
	{
		return;
	}

	if (mesh)
	{
		if (instance.skining_.skin_index_ == MODEL_INVALID)
		{
			float chunk_scale = max(max(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz)), length(instance.transform_.world_[2].xyz));
			float3 chunk_center = mul(float4(instance.streaming_.position_min_ + instance.streaming_.position_extent_ * 0.5, 1.0), instance.transform_.world_).xyz;
			float chunk_radius = length(instance.streaming_.position_extent_) * 0.5 * chunk_scale;
			if (!IsInsideFrustum(chunk_center, chunk_radius, scene.current_view_projection_))
			{
				return;
			}
			if (IsOccluded(chunk_center, chunk_radius, scene.current_view_projection_, scene.camera_position_.xyz, scene.projection_._m11, scene.screen_size_, transparent_bindings.geometry_hi_z_))
			{
				return;
			}
		}

		RWStructuredBuffer<uint> chunk_arguments = ResourceDescriptorHeap[transparent_bindings.culling_transparent_arguments_];
		RWStructuredBuffer<uint> transparent_chunks = ResourceDescriptorHeap[transparent_bindings.culling_transparent_chunks_];
		uint chunk_slot;
		InterlockedAdd(chunk_arguments[CULLING_TRANSPARENT_COUNT], 1, chunk_slot);
		transparent_chunks[chunk_slot] = instance_index;
		InterlockedMax(chunk_arguments[1], chunk_slot / 1024 + 1);
		return;
	}

	uint meshlet_index = instance.geometry_.meshlet_offset_ + meshlet_local;

	if (instance.skining_.skin_index_ == MODEL_INVALID)
	{
		StructuredBuffer<ModelMeshletBound> bounds = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.meshlet_bound_buffer_index_)];
		ModelMeshletBound bound = bounds[meshlet_index];
		float world_scale = max(max(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz)), length(instance.transform_.world_[2].xyz));
		float3 center = mul(float4(bound.center_, 1.0), instance.transform_.world_).xyz;
		float radius = bound.radius_ * world_scale;

		if (!IsInsideFrustum(center, radius, scene.current_view_projection_))
		{
			return;
		}
		if (instance.shading_.double_sided_ == 0)
		{
			float3 cone_axis = normalize(mul(float4(bound.cone_axis_, 0.0), instance.transform_.world_).xyz);
			if (IsBackfacing(center, radius, cone_axis, bound.cone_cutoff_, scene.camera_position_.xyz))
			{
				return;
			}
		}
		if (IsOccluded(center, radius, scene.current_view_projection_, scene.camera_position_.xyz, scene.projection_._m11, scene.screen_size_, transparent_bindings.geometry_hi_z_))
		{
			return;
		}
	}

	RWStructuredBuffer<uint> arguments = ResourceDescriptorHeap[transparent_bindings.culling_transparent_arguments_];
	RWStructuredBuffer<uint2> transparent_meshlets = ResourceDescriptorHeap[transparent_bindings.culling_transparent_meshlets_];
	uint capacity;
	uint stride;
	transparent_meshlets.GetDimensions(capacity, stride);
	uint slot;
	InterlockedAdd(arguments[CULLING_TRANSPARENT_COUNT], 1, slot);
	if (slot >= capacity)
	{
		InterlockedAdd(arguments[CULLING_TRANSPARENT_OVERFLOW], 1);
		return;
	}
	transparent_meshlets[slot] = uint2(instance_index, meshlet_index);
	InterlockedMax(arguments[1], slot + 1);
}
