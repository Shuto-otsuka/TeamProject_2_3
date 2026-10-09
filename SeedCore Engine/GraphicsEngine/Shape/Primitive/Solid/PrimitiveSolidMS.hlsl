#include "PrimitiveSolid.hlsli"
#include "../../../Shader/ShaderResources.hlsli"
#include "../../../Shader/Culling.hlsli"

groupshared float4 clip_positions[64];

[NumThreads(64, 1, 1)]
[OutputTopology("triangle")]
void main(in payload PrimitiveSolidASPayload as_payload, uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, out vertices PrimitiveSolidMSOutput verts[64], out indices uint3 triangles[124])
{
    PrimitiveSolidShaderResourceIndices solid = shader_resource_indices.primitive_solid_;
    StructuredBuffer<PrimitiveSolidStructuredBuffer> instances = GetPrimitiveSolidStructuredBuffer(solid.instance_index_);
    StructuredBuffer<PrimitiveMeshlet> meshlets = ResourceDescriptorHeap[solid.meshlet_index_];
    StructuredBuffer<PrimitiveVertex> mesh_vertices = ResourceDescriptorHeap[solid.vertex_index_];
    StructuredBuffer<uint> vertex_indices = ResourceDescriptorHeap[solid.vertex_indices_index_];
    ByteAddressBuffer primitive_indices = ResourceDescriptorHeap[solid.primitive_indices_index_];
    SceneConstantBuffer scene = GetSceneConstantBuffer();

    uint instance_index = as_payload.instance_index;
    PrimitiveSolidStructuredBuffer instance = instances[instance_index];
    PrimitiveMeshlet meshlet = meshlets[as_payload.meshlet_indices[gid]];

    SetMeshOutputCounts(meshlet.vertex_count_, meshlet.triangle_count_);

    if (gtid < meshlet.vertex_count_)
    {
        PrimitiveVertex vertex = mesh_vertices[vertex_indices[meshlet.vertex_offset_ + gtid]];

        /// [EN] Scale the unit mesh to the instance (Box: half extents), rotate it by the instance quaternion (v + w * t + cross(xyz, t) with t = 2 * cross(xyz, v)), then move it. The normal is divided by the scale before rotating so it stays perpendicular under non-uniform scaling.
		/// [JP] 単位メッシュをインスタンスの大きさにし（Box は半分の大きさ）、インスタンスのクォータニオンで回して（t = 2 * cross(xyz, v) として v + w * t + cross(xyz, t)）、位置へ動かす。法線は、拡縮がそろっていなくても面に垂直なままになるよう、拡縮で割ってから回す。
        float3 scale = instance.dimensions_;
        float3 local_position = vertex.position_ * scale;
        float3 position_twist = 2.0 * cross(instance.rotation_.xyz, local_position);
        float3 world_position = local_position + instance.rotation_.w * position_twist + cross(instance.rotation_.xyz, position_twist) + instance.position_;

        float3 local_normal = normalize(vertex.normal_ / scale);
        float3 normal_twist = 2.0 * cross(instance.rotation_.xyz, local_normal);
        float3 world_normal = local_normal + instance.rotation_.w * normal_twist + cross(instance.rotation_.xyz, normal_twist);

        PrimitiveSolidMSOutput output;
        output.position = mul(float4(world_position, 1.0), scene.current_view_projection_);
        output.normal = world_normal;
        output.texcoord = vertex.texcoord_ * instance.uv_scale_ + instance.uv_offset_;
        output.instance_index = instance_index;

        verts[gtid] = output;
        clip_positions[gtid] = output.position;
    }

    GroupMemoryBarrierWithGroupSync();

    /// [EN] 64 threads cover up to 124 triangles, so each thread loops. The primitive indices hold one byte per corner; single-sided shapes drop back faces here, as the model mesh shaders do.
	/// [JP] 64 スレッドで最大 124 三角形を扱うので、各スレッドがループする。三角形番号は角1つにつき1バイト。片面の形は、モデルのメッシュシェーダーと同じくここで裏面を捨てる。
    for (uint triangle_index = gtid; triangle_index < meshlet.triangle_count_; triangle_index += 64)
    {
        uint3 corners;
        for (uint corner = 0; corner < 3; corner++)
        {
            uint byte_offset = meshlet.triangle_offset_ + triangle_index * 3 + corner;
            corners[corner] = (primitive_indices.Load(byte_offset & ~3) >> ((byte_offset & 3) * 8)) & 0xFF;
        }

        if (instance.double_sided_ == 0 && IsBackFace(clip_positions[corners.x], clip_positions[corners.y], clip_positions[corners.z]))
        {
            triangles[triangle_index] = uint3(0, 0, 0);
        }
        else
        {
            triangles[triangle_index] = corners;
        }
    }
}