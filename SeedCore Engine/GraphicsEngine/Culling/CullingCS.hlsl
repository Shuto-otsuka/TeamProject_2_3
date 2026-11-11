#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Culling/FrustumCulling.hlsli>
#include <GraphicsEngine/Culling/BackfaceCulling.hlsli>
#include <GraphicsEngine/Culling/OcclusionCulling.hlsli>
#include <GraphicsEngine/Culling/Culling.hlsli>

struct CullingEarlyBindings
{
	uint model_instances_;
	uint model_counts_;
	uint geometry_hierarchical_depth_history_;
	uint culling_rejected_;
	uint culling_draw_lists_;
	uint culling_draw_arguments_;
};
ConstantBuffer<CullingEarlyBindings> early_bindings : register(b0, space1);

struct CullingLateBindings
{
	uint model_instances_;
	uint model_counts_;
	uint geometry_hierarchical_depth_;
	uint culling_rejected_;
	uint culling_draw_lists_;
	uint culling_draw_arguments_;
};
ConstantBuffer<CullingLateBindings> late_bindings : register(b0, space1);

/**
* [EN]
* First dispatch of Culling.CameraEarly (one thread). Clears the counts and
* writes the fixed parts of the early and late indirect commands. The top bit
* of the dispatch constant selects the layout: DispatchMesh with 1024 groups
* per row (the row count grows as meshlets are appended), or the fallback
* DrawInstanced with 124 triangles' worth of vertices per meshlet instance.
*/
[numthreads(1, 1, 1)]
void Prepare(uint3 id : SV_DispatchThreadID)
{
	RWStructuredBuffer<uint> arguments = ResourceDescriptorHeap[early_bindings.culling_draw_arguments_];
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
}

/**
* [EN]
* One thread per meshlet of the opaque section of ModelInstances (the low
* bits of the dispatch constant are the section's end). Frustum and backface
* rejections are final; a meshlet that survives them but is hidden by the
* previous frame's Hi-Z goes to Culling.Rejected so the late pass can test it
* again against this frame's depth (when Culling.Rejected is full it is drawn
* early instead). Skinned meshlets have rest-pose bounds, so they skip every
* test. Each visible meshlet widens the early command so it covers its slot.
*/
[numthreads(64, 1, 1)]
void Early(uint3 id : SV_DispatchThreadID)
{
	uint section_end = dispatch_bindings.constants_ & 0x7FFFFFFF;
	bool mesh = (dispatch_bindings.constants_ & 0x80000000) != 0;
	uint instance_index = id.x / MODEL_MESHLETS_PER_INSTANCE;
	uint meshlet_local = id.x % MODEL_MESHLETS_PER_INSTANCE;

	StructuredBuffer<uint> counts = ResourceDescriptorHeap[early_bindings.model_counts_];
	if (instance_index >= min(counts[MODEL_COUNT_OPAQUE], section_end))
	{
		return;
	}

	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[early_bindings.model_instances_];
	ModelStructuredBuffer instance = instances[instance_index];
	if (meshlet_local >= instance.geometry_.meshlet_count_)
	{
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	RWStructuredBuffer<uint> arguments = ResourceDescriptorHeap[early_bindings.culling_draw_arguments_];
	uint meshlet_index = instance.geometry_.meshlet_offset_ + meshlet_local;
	uint2 entry = uint2(instance_index, meshlet_index);

	if (instance.skining_.skin_index_ == MODEL_INVALID)
	{
		StructuredBuffer<ModelMeshletBound> bounds = ResourceDescriptorHeap[instance.geometry_.meshlet_bound_buffer_index_];
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
		if (IsOccluded(center, radius, scene.previous_view_projection_, scene.camera_position_.xyz, scene.projection_._m11, scene.screen_size_, early_bindings.geometry_hierarchical_depth_history_))
		{
			RWStructuredBuffer<uint2> rejected = ResourceDescriptorHeap[early_bindings.culling_rejected_];
			uint rejected_capacity;
			uint rejected_stride;
			rejected.GetDimensions(rejected_capacity, rejected_stride);
			uint rejected_slot;
			InterlockedAdd(arguments[CULLING_REJECTED_COUNT], 1, rejected_slot);
			if (rejected_slot < rejected_capacity)
			{
				rejected[rejected_slot] = entry;
				return;
			}
		}
	}

	RWStructuredBuffer<uint2> draw_lists = ResourceDescriptorHeap[early_bindings.culling_draw_lists_];
	uint capacity;
	uint stride;
	draw_lists.GetDimensions(capacity, stride);
	uint slot;
	InterlockedAdd(arguments[CULLING_EARLY_COUNT], 1, slot);
	if (slot >= capacity)
	{
		InterlockedAdd(arguments[CULLING_OVERFLOW], 1);
		return;
	}
	draw_lists[slot] = entry;
	if (mesh)
	{
		InterlockedMax(arguments[CULLING_EARLY_ARGUMENTS * 3 + 1], slot / 1024 + 1);
	}
	else
	{
		InterlockedMax(arguments[CULLING_EARLY_ARGUMENTS * 4 + 1], slot + 1);
	}
}

/**
* [EN]
* One thread per entry of Culling.Rejected. Tests the meshlet again against
* the Hi-Z built from this frame's early depth, and appends the ones now
* visible right after the early entries of the draw list, widening the late
* command the same way as Early.
*/
[numthreads(64, 1, 1)]
void Late(uint3 id : SV_DispatchThreadID)
{
	bool mesh = (dispatch_bindings.constants_ & 0x80000000) != 0;
	RWStructuredBuffer<uint> arguments = ResourceDescriptorHeap[late_bindings.culling_draw_arguments_];
	StructuredBuffer<uint2> rejected = ResourceDescriptorHeap[late_bindings.culling_rejected_];
	uint rejected_capacity;
	uint rejected_stride;
	rejected.GetDimensions(rejected_capacity, rejected_stride);
	if (id.x >= min(arguments[CULLING_REJECTED_COUNT], rejected_capacity))
	{
		return;
	}

	uint2 entry = rejected[id.x];
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[late_bindings.model_instances_];
	ModelStructuredBuffer instance = instances[entry.x];
	StructuredBuffer<ModelMeshletBound> bounds = ResourceDescriptorHeap[instance.geometry_.meshlet_bound_buffer_index_];
	ModelMeshletBound bound = bounds[entry.y];
	float world_scale = max(max(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz)), length(instance.transform_.world_[2].xyz));
	float3 center = mul(float4(bound.center_, 1.0), instance.transform_.world_).xyz;
	float radius = bound.radius_ * world_scale;

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	if (IsOccluded(center, radius, scene.current_view_projection_, scene.camera_position_.xyz, scene.projection_._m11, scene.screen_size_, late_bindings.geometry_hierarchical_depth_))
	{
		return;
	}

	RWStructuredBuffer<uint2> draw_lists = ResourceDescriptorHeap[late_bindings.culling_draw_lists_];
	uint capacity;
	uint stride;
	draw_lists.GetDimensions(capacity, stride);
	uint base = min(arguments[CULLING_EARLY_COUNT], capacity);
	uint slot;
	InterlockedAdd(arguments[CULLING_LATE_COUNT], 1, slot);
	if (base + slot >= capacity)
	{
		InterlockedAdd(arguments[CULLING_OVERFLOW], 1);
		return;
	}
	draw_lists[base + slot] = entry;
	if (mesh)
	{
		InterlockedMax(arguments[CULLING_LATE_ARGUMENTS * 3 + 1], slot / 1024 + 1);
	}
	else
	{
		InterlockedMax(arguments[CULLING_LATE_ARGUMENTS * 4 + 1], slot + 1);
	}
}
