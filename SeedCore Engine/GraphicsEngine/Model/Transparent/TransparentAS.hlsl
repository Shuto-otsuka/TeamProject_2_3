#include <GraphicsEngine/Model/Transparent/Transparent.hlsli>
#include <GraphicsEngine/Culling/FrustumCulling.hlsli>
#include <GraphicsEngine/Culling/BackfaceCulling.hlsli>
#include <GraphicsEngine/Culling/OcclusionCulling.hlsli>

groupshared TransparentPayload payload;
groupshared uint survivor_count;

[NumThreads(32, 1, 1)]
void main(uint gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
	if (gtid == 0)
	{
		survivor_count = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	uint group_index = gid.y * 1024 + gid.x;
	StructuredBuffer<uint> arguments = ResourceDescriptorHeap[bindings.culling_transparent_arguments_];
	bool visible = false;
	uint2 entry = uint2(0, 0);
	if (group_index < arguments[CULLING_TRANSPARENT_COUNT])
	{
		StructuredBuffer<uint> chunks = ResourceDescriptorHeap[bindings.culling_transparent_chunks_];
		StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
		uint instance_index = chunks[group_index];
		ModelStructuredBuffer instance = instances[instance_index];
		if (gtid < instance.geometry_.meshlet_count_)
		{
			entry = uint2(instance_index, instance.geometry_.meshlet_offset_ + gtid);
			visible = true;
			if (instance.skining_.skin_index_ == MODEL_INVALID)
			{
				SceneConstantBuffer scene = GetSceneConstantBuffer();
				StructuredBuffer<ModelMeshletBound> bounds = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.meshlet_bound_buffer_index_)];
				ModelMeshletBound bound = bounds[entry.y];
				float world_scale = max(max(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz)), length(instance.transform_.world_[2].xyz));
				float3 center = mul(float4(bound.center_, 1.0), instance.transform_.world_).xyz;
				float radius = bound.radius_ * world_scale;

				visible = IsInsideFrustum(center, radius, scene.current_view_projection_);
				if (visible && instance.shading_.double_sided_ == 0)
				{
					float3 cone_axis = normalize(mul(float4(bound.cone_axis_, 0.0), instance.transform_.world_).xyz);
					visible = !IsBackfacing(center, radius, cone_axis, bound.cone_cutoff_, scene.camera_position_.xyz);
				}
				if (visible)
				{
					visible = !IsOccluded(center, radius, scene.current_view_projection_, bindings.geometry_hi_z_);
				}
			}
		}
	}

	if (visible)
	{
		uint survivor_slot;
		InterlockedAdd(survivor_count, 1, survivor_slot);
		payload.entries_[survivor_slot] = entry;
	}
	GroupMemoryBarrierWithGroupSync();

	DispatchMesh(survivor_count, 1, 1, payload);
}
