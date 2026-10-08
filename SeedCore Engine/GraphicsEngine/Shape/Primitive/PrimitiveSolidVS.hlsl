#include "PrimitiveSolid.hlsli"
#include "../../Shader/ShaderResources.hlsli"
#include "../../Shader/Dispatch.hlsli"

PrimitiveSolidMSOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	PrimitiveSolidMSOutput output = (PrimitiveSolidMSOutput)0;
	output.position = float4(2.0, 2.0, 2.0, 1.0);

	PrimitiveSolidShaderResourceIndices solid = shader_resource_indices.primitive_solid_;
    StructuredBuffer<PrimitiveSolidCullingStructuredBuffer> culling = GetPrimitiveSolidCullingStructuredBuffer(dispatch_buffer_index_);
	StructuredBuffer<PrimitiveMeshlet> meshlets = ResourceDescriptorHeap[solid.meshlet_index_];

	PrimitiveSolidCullingStructuredBuffer visible = culling[instance_id];
	PrimitiveMeshlet meshlet = meshlets[visible.meshlet_index_];
	if (vertex_id / 3 >= meshlet.triangle_count_)
	{
		return output;
	}

    StructuredBuffer<PrimitiveSolidStructuredBuffer> instances = GetPrimitiveSolidStructuredBuffer(solid.instance_index_);
	StructuredBuffer<PrimitiveVertex> mesh_vertices = ResourceDescriptorHeap[solid.vertex_index_];
	StructuredBuffer<uint> vertex_indices = ResourceDescriptorHeap[solid.vertex_indices_index_];
	ByteAddressBuffer primitive_indices = ResourceDescriptorHeap[solid.primitive_indices_index_];
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	/// [EN] vertex_id is already triangle * 3 + corner, so it is the byte offset within the meshlet's primitive indices.
	/// [JP] vertex_id はそのまま「三角形番号 × 3 ＋ 角」なので、メッシュレットの三角形番号の中でのバイト位置になる。
	uint byte_offset = meshlet.triangle_offset_ + vertex_id;
	uint local_index = (primitive_indices.Load(byte_offset & ~3) >> ((byte_offset & 3) * 8)) & 0xFF;
	PrimitiveVertex vertex = mesh_vertices[vertex_indices[meshlet.vertex_offset_ + local_index]];
	PrimitiveSolidStructuredBuffer instance = instances[visible.instance_index_];

	/// [EN] Same transform as PrimitiveSolidMS.hlsl.
	/// [JP] PrimitiveSolidMS.hlsl と同じ変換。
	float3 scale = instance.dimensions_;
	float3 local_position = vertex.position_ * scale;
	float3 position_twist = 2.0 * cross(instance.rotation_.xyz, local_position);
	float3 world_position = local_position + instance.rotation_.w * position_twist + cross(instance.rotation_.xyz, position_twist) + instance.position_;

	float3 local_normal = normalize(vertex.normal_ / scale);
	float3 normal_twist = 2.0 * cross(instance.rotation_.xyz, local_normal);
	float3 world_normal = local_normal + instance.rotation_.w * normal_twist + cross(instance.rotation_.xyz, normal_twist);

	output.position = mul(float4(world_position, 1.0), scene.current_view_projection_);
	output.normal = world_normal;
	output.texcoord = vertex.texcoord_ * instance.uv_scale_ + instance.uv_offset_;
	output.instance_index = visible.instance_index_;
	return output;
}