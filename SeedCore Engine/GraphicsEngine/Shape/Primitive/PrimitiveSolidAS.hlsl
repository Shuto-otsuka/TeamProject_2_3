#include "PrimitiveSolid.hlsli"
#include "../../Shader/ShaderResources.hlsli"
#include "../../Shader/Culling.hlsli"

groupshared uint survived_count;
groupshared PrimitiveSolidASPayload payload;

/**
* [EN]
* One group per instance, one thread per meshlet of its shape. Meshlets
* whose bounding sphere is outside the view frustum are dropped, and the
* survivors are handed to the mesh shader, as ModelAS.hlsl does.
*
* ---------------------------------------------------------------------
*
* [JP]
* 1 グループが 1 インスタンスを、1 スレッドがその形のメッシュレット 1 つを
* 担当する。包囲球が視錐台の外にあるメッシュレットを捨て、残ったものを
* メッシュシェーダーへ渡す。ModelAS.hlsl と同じ形。
*/
[numthreads(32, 1, 1)]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
    if (gtid == 0)
    {
        survived_count = 0;
        payload.instance_index = gid;
    }

    GroupMemoryBarrierWithGroupSync();

    PrimitiveSolidShaderResourceIndices solid = shader_resource_indices.primitive_solid_;
    StructuredBuffer<PrimitiveSolidStructuredBuffer> instances = GetPrimitiveSolidStructuredBuffer(solid.instance_index_);
    StructuredBuffer<PrimitiveMeshletBound> bounds = ResourceDescriptorHeap[solid.meshlet_bound_index_];
    SceneConstantBuffer scene = GetSceneConstantBuffer();

    PrimitiveSolidStructuredBuffer instance = instances[gid];
    if (gtid < instance.meshlet_count_)
    {
        PrimitiveMeshletBound bound = bounds[instance.meshlet_offset_ + gtid];

        /// [EN] Move the bounding sphere like the instance; the radius follows the largest axis so the test stays conservative under non-uniform scaling.
		/// [JP] 包囲球をインスタンスと同じく動かす。拡縮がそろっていなくても判定が甘くならないよう、半径は一番大きい軸に合わせる。
        float3 scale = instance.dimensions_;
        float3 local_center = bound.center_ * scale;
        float3 twist = 2.0 * cross(instance.rotation_.xyz, local_center);
        float3 world_center = local_center + instance.rotation_.w * twist + cross(instance.rotation_.xyz, twist) + instance.position_;
        float world_radius = bound.radius_ * max(max(scale.x, scale.y), scale.z);

        if (IsVisibleInFrustum(world_center, world_radius, scene.current_view_projection_))
        {
            uint slot;
            InterlockedAdd(survived_count, 1, slot);
            payload.meshlet_indices[slot] = instance.meshlet_offset_ + gtid;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    DispatchMesh(survived_count, 1, 1, payload);
}