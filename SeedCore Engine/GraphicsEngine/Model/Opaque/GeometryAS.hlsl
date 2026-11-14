#include <GraphicsEngine/Model/Opaque/Geometry.hlsli>
#include <GraphicsEngine/Culling/FrustumCulling.hlsli>
#include <GraphicsEngine/Culling/BackfaceCulling.hlsli>
#include <GraphicsEngine/Culling/OcclusionCulling.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

groupshared GeometryPayload payload;
groupshared uint survivor_count;

/**
* [EN]
* One thread group per chunk (or per 32 entries of Geometry.OccludedMeshlets), one
* thread per meshlet; the groups past the list's count (the dispatch is
* rounded up to whole rows of 1024) launch nothing. The low byte of the
* dispatch constant picks the list: the early draw tests the meshlets of
* Culling.OpaqueChunks against the frustum, their normal cone and the previous
* frame's Hi-Z, and records the ones only that Hi-Z hides in Geometry.OccludedMeshlets
* (they are drawn early when it is full); the late draws test the meshlets of
* Culling.OccludedChunks the same way against this frame's Hi-Z, and the
* entries of Geometry.OccludedMeshlets against this frame's Hi-Z alone. The replay bit
* repeats a depth draw's decisions for a later redraw without recording
* anything; an occluded early meshlet is drawn by the replay only when
* Geometry.OccludedMeshlets overflowed, which the depth test then sorts out. Skinned
* instances have rest-pose bounds, so they skip every test. Every group ends
* in exactly one DispatchMesh, launching one GeometryMS group per survivor.
*/
[NumThreads(32, 1, 1)]
void main(uint gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
	if (gtid == 0)
	{
		survivor_count = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	uint command = dispatch_bindings.constants_ & CULLING_COMMAND_MASK;
	bool replay = (dispatch_bindings.constants_ & CULLING_REPLAY) != 0;
	uint group_index = gid.y * 1024 + gid.x;
	SceneConstantBuffer scene = GetSceneConstantBuffer();
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
	RWStructuredBuffer<uint> occluded_meshlets = ResourceDescriptorHeap[bindings.geometry_occluded_meshlets_];
	uint occluded_capacity;
	uint occluded_stride;
	occluded_meshlets.GetDimensions(occluded_capacity, occluded_stride);
	occluded_capacity = (occluded_capacity - 1) / 2;

	bool visible = false;
	uint2 entry = uint2(0, 0);
	if (command == CULLING_OCCLUDED_ARGUMENTS)
	{
		uint occluded_index = group_index * 32 + gtid;
		if (occluded_index < min(occluded_meshlets[0], occluded_capacity))
		{
			entry = uint2(occluded_meshlets[1 + occluded_index * 2], occluded_meshlets[2 + occluded_index * 2]);
			ModelStructuredBuffer instance = instances[entry.x];
			visible = true;
			if (instance.skining_.skin_index_ == MODEL_INVALID)
			{
				StructuredBuffer<ModelMeshletBound> bounds = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.meshlet_bound_buffer_index_)];
				ModelMeshletBound bound = bounds[entry.y];
				float world_scale = max(max(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz)), length(instance.transform_.world_[2].xyz));
				float3 center = mul(float4(bound.center_, 1.0), instance.transform_.world_).xyz;
				float radius = bound.radius_ * world_scale;
				if (instance.geometry_.layer_ != 0)
				{
					radius += instance.shading_.fur_length_ * (world_scale + 0.35);
				}
				visible = !IsOccluded(center, radius, scene.current_view_projection_, scene.camera_position_.xyz, scene.projection_._m11, scene.screen_size_, bindings.geometry_hi_z_);
			}
		}
	}
	else
	{
		bool late = command == CULLING_LATE_ARGUMENTS;
		StructuredBuffer<uint> chunk_arguments = ResourceDescriptorHeap[bindings.culling_chunk_arguments_];
		StructuredBuffer<uint> chunks = ResourceDescriptorHeap[late ? bindings.culling_occluded_chunks_ : bindings.culling_opaque_chunks_];
		if (group_index < chunk_arguments[late ? CULLING_OCCLUDED_CHUNK_COUNT : CULLING_OPAQUE_CHUNK_COUNT])
		{
			uint instance_index = chunks[group_index];
			ModelStructuredBuffer instance = instances[instance_index];
			if (gtid < instance.geometry_.meshlet_count_)
			{
				entry = uint2(instance_index, instance.geometry_.meshlet_offset_ + gtid);
				visible = true;
				if (instance.skining_.skin_index_ == MODEL_INVALID)
				{
					StructuredBuffer<ModelMeshletBound> bounds = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.meshlet_bound_buffer_index_)];
					ModelMeshletBound bound = bounds[entry.y];
					float world_scale = max(max(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz)), length(instance.transform_.world_[2].xyz));
					float3 center = mul(float4(bound.center_, 1.0), instance.transform_.world_).xyz;
					float radius = bound.radius_ * world_scale;
					if (instance.geometry_.layer_ != 0)
					{
						radius += instance.shading_.fur_length_ * (world_scale + 0.35);
					}

					visible = IsInsideFrustum(center, radius, scene.current_view_projection_);
					if (visible && instance.shading_.double_sided_ == 0)
					{
						float3 cone_axis = normalize(mul(float4(bound.cone_axis_, 0.0), instance.transform_.world_).xyz);
						visible = !IsBackfacing(center, radius, cone_axis, bound.cone_cutoff_, scene.camera_position_.xyz);
					}
					if (visible && late)
					{
						visible = !IsOccluded(center, radius, scene.current_view_projection_, scene.camera_position_.xyz, scene.projection_._m11, scene.screen_size_, bindings.geometry_hi_z_);
					}
					else if (visible && IsOccluded(center, radius, scene.previous_view_projection_, scene.camera_position_.xyz, scene.projection_._m11, scene.screen_size_, bindings.geometry_hi_z_history_))
					{
						if (replay)
						{
							visible = occluded_meshlets[0] > occluded_capacity;
						}
						else
						{
							uint occluded_slot;
							InterlockedAdd(occluded_meshlets[0], 1, occluded_slot);
							if (occluded_slot < occluded_capacity)
							{
								occluded_meshlets[1 + occluded_slot * 2] = entry.x;
								occluded_meshlets[2 + occluded_slot * 2] = entry.y;
								visible = false;
							}
						}
					}
				}
			}
		}
	}

	if (visible)
	{
		uint survivor_slot;
		InterlockedAdd(survivor_count, 1, survivor_slot);
		payload.entries_[survivor_slot] = entry;
		if (!replay && scene.view_mode_ == VIEW_MODE_MESHLET_BOUNDS)
		{
			RWStructuredBuffer<uint> visible_meshlets = ResourceDescriptorHeap[bindings.geometry_visible_meshlets_];
			uint visible_meshlets_capacity;
			uint visible_meshlets_stride;
			visible_meshlets.GetDimensions(visible_meshlets_capacity, visible_meshlets_stride);
			uint visible_meshlets_slot;
			InterlockedAdd(visible_meshlets[0], 1, visible_meshlets_slot);
			if (visible_meshlets_slot < (visible_meshlets_capacity - 1) / 2)
			{
				visible_meshlets[1 + visible_meshlets_slot * 2] = entry.x;
				visible_meshlets[2 + visible_meshlets_slot * 2] = entry.y;
			}
		}
	}
	GroupMemoryBarrierWithGroupSync();

	DispatchMesh(survivor_count, 1, 1, payload);
}
