#include <GraphicsEngine/Model/Transparent/Transparent.hlsli>
#include <GraphicsEngine/Culling/FrustumCulling.hlsli>
#include <GraphicsEngine/Culling/BackfaceCulling.hlsli>
#include <GraphicsEngine/Culling/OcclusionCulling.hlsli>

groupshared TransparentPayload payload;
groupshared uint survivor_count;

/**
* [EN]
* One thread group per chunk of Culling.TransparentChunks, one thread per
* meshlet; the groups past the list's count (the dispatch is rounded up to
* whole rows of 1024) launch nothing. Each meshlet is tested against the
* frustum, its normal cone and the Hi-Z built from this frame's final depth;
* skinned instances have rest-pose bounds, so they skip every test. Every
* group ends in exactly one DispatchMesh, launching one TransparentMS group
* per survivor.
*
* ---------------------------------------------------------------------
*
* [JP]
* Culling.TransparentChunks のチャンク 1 つにつき 1 スレッドグループ、meshlet 1 つに
* つき 1 スレッド。一覧の件数より後ろのグループ（ディスパッチは 1024 の行単位に
* 切り上がる）は何も起動しない。各 meshlet を視錐台・法線コーン・このフレームの
* 最終の深度から作った Hi-Z で判定する。スキンのインスタンスは静止姿勢の境界しか
* 持たないので、どの判定もしない。どのグループも最後に DispatchMesh をちょうど
* 1 回呼び、残った 1 件につき TransparentMS を 1 グループ起動する。
*/
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
					visible = !IsOccluded(center, radius, scene.current_view_projection_, scene.camera_position_.xyz, scene.projection_._m11, scene.screen_size_, bindings.geometry_hi_z_);
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
