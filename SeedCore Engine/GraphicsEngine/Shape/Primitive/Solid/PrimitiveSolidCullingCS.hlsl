#include "PrimitiveSolid.hlsli"
#include "../../../Shader/ShaderResources.hlsli"
#include "../../../Shader/Culling.hlsli"
#include "../../../Shader/Dispatch.hlsli"

[numthreads(32, 1, 1)]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
    PrimitiveSolidShaderResourceIndices solid = shader_resource_indices.primitive_solid_;
    StructuredBuffer<PrimitiveSolidStructuredBuffer> instances = GetPrimitiveSolidStructuredBuffer(solid.instance_index_);
    StructuredBuffer<PrimitiveMeshletBound> bounds = ResourceDescriptorHeap[solid.meshlet_bound_index_];
    SceneConstantBuffer scene = GetSceneConstantBuffer();

    PrimitiveSolidStructuredBuffer instance = instances[gid];
    if (gtid >= instance.meshlet_count_)
    {
        return;
    }

    /// [EN] Same frustum test as PrimitiveSolidAS.hlsl.
	/// [JP] PrimitiveSolidAS.hlsl と同じ視錐台の判定。
    PrimitiveMeshletBound bound = bounds[instance.meshlet_offset_ + gtid];
    float3 scale = instance.dimensions_;
    float3 local_center = bound.center_ * scale;
    float3 twist = 2.0 * cross(instance.rotation_.xyz, local_center);
    float3 world_center = local_center + instance.rotation_.w * twist + cross(instance.rotation_.xyz, twist) + instance.position_;
    float world_radius = bound.radius_ * max(max(scale.x, scale.y), scale.z);
    if (!IsVisibleInFrustum(world_center, world_radius, scene.current_view_projection_))
    {
        return;
    }

    ConstantBuffer<PrimitiveSolidCullingConstantBuffer> culling = GetPrimitiveSolidCullingConstantBuffer();
    RWByteAddressBuffer arguments = ResourceDescriptorHeap[culling.arguments_index_];

    PrimitiveSolidCullingStructuredBuffer visible = (PrimitiveSolidCullingStructuredBuffer)0;
    visible.instance_index_ = gid;
    visible.meshlet_index_ = instance.meshlet_offset_ + gtid;

    /// [EN] Each list's instance count is 4 bytes into its D3D12_DRAW_ARGUMENTS: single-sided starts at 0, double-sided at 16.
	/// [JP] 各一覧のインスタンス数は、その D3D12_DRAW_ARGUMENTS の 4 バイト目にある。片面用は 0、両面用は 16 から始まる。
    uint slot;
    if (instance.double_sided_ == 0)
    {
        arguments.InterlockedAdd(4, 1, slot);
        RWStructuredBuffer<PrimitiveSolidCullingStructuredBuffer> single_sided = ResourceDescriptorHeap[culling.single_sided_index_];
        single_sided[slot] = visible;
    }
    else
    {
        arguments.InterlockedAdd(20, 1, slot);
        RWStructuredBuffer<PrimitiveSolidCullingStructuredBuffer> double_sided = ResourceDescriptorHeap[culling.double_sided_index_];
        double_sided[slot] = visible;
    }
}