#include <GraphicsEngine/Preview/Preview.hlsli>
#include <GraphicsEngine/Culling/BackfaceCulling.hlsli>

/**
* [EN]
* Preview.Draw.Mesh for devices without mesh shaders: one DrawInstanced
* instance per entry of Preview.Meshlets, 124 triangles' worth of vertices
* each, placed like PreviewMS. Vertices past the meshlet's triangle count go
* outside the clip volume, and for a single-sided material every vertex
* transforms all three corners of its triangle so it can collapse a
* back-facing one itself.
*
* ---------------------------------------------------------------------
*
* [JP]
* メッシュシェーダーが使えないデバイス向けの Preview.Draw.Mesh。Preview.Meshlets の
* 1 件につき DrawInstanced の 1 インスタンスで、三角形 124 個分の頂点を PreviewMS と
* 同じ位置に描く。meshlet の三角形数を超えた頂点はクリップ空間の外へ置き、片面の
* マテリアルでは各頂点が三角形の 3 つの角を変換して、裏を向く三角形を自分で潰す。
*/
PreviewOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	StructuredBuffer<uint2> entries = ResourceDescriptorHeap[mesh_bindings.preview_meshlets_];
	uint2 entry = entries[instance_id];
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[mesh_bindings.preview_instances_];
	ModelStructuredBuffer instance = instances[entry.x];
	StructuredBuffer<ModelMeshlet> meshlets = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.meshlet_buffer_index_)];
	ModelMeshlet meshlet = meshlets[entry.y];

	uint triangle_index = vertex_id / 3;
	uint corner = vertex_id % 3;
	PreviewOutput output = (PreviewOutput)0;
	output.position = float4(2.0, 2.0, 2.0, 1.0);
	output.instance_index = entry.x;
	if (triangle_index >= meshlet.triangle_count_)
	{
		return output;
	}

	StructuredBuffer<CompressedModelVertex> vertices = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.vertex_buffer_index_)];
	StructuredBuffer<uint> vertex_indices = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.vertex_indices_buffer_index_)];
	ByteAddressBuffer primitive_indices = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.primitive_indices_buffer_index_)];
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	uint byte_offset = meshlet.triangle_offset_ + triangle_index * 3;
	uint aligned_offset = byte_offset & ~3;
	uint shift = (byte_offset & 3) * 8;
	uint packed = primitive_indices.Load(aligned_offset) >> shift;
	if (shift > 8)
	{
		packed |= primitive_indices.Load(aligned_offset + 4) << (32 - shift);
	}

	bool single_sided = instance.shading_.double_sided_ == 0;
	float4 clip_positions[3] = { float4(0.0, 0.0, 0.0, 1.0), float4(0.0, 0.0, 0.0, 1.0), float4(0.0, 0.0, 0.0, 1.0) };
	for (uint corner_index = 0; corner_index < 3; corner_index++)
	{
		if (!single_sided && corner_index != corner)
		{
			continue;
		}

		uint global_vertex_index = vertex_indices[meshlet.vertex_offset_ + ((packed >> (corner_index * 8)) & 0xFF)];
		ExpandedModelVertex vertex = DecodeModelVertex(vertices[global_vertex_index], instance);
		float3 local_position = vertex.position_;

		if (instance.skining_.skin_index_ != MODEL_INVALID)
		{
			StructuredBuffer<ModelBoneMatrix> bone_matrices = ResourceDescriptorHeap[mesh_bindings.preview_bones_];
			StructuredBuffer<CompressedModelSkin> skin_vertices = ResourceDescriptorHeap[NonUniformResourceIndex(instance.skining_.skin_vertex_buffer_index_)];
			ExpandedModelSkin skin = DecodeSkinVertex(skin_vertices[global_vertex_index]);
			float4x4 skin_matrix =
				LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin.joints_.x]) * skin.weights_.x +
				LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin.joints_.y]) * skin.weights_.y +
				LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin.joints_.z]) * skin.weights_.z +
				LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin.joints_.w]) * skin.weights_.w;
			local_position = mul(float4(local_position, 1.0), skin_matrix).xyz;
		}

		clip_positions[corner_index] = mul(mul(float4(local_position, 1.0), instance.transform_.world_), scene.current_view_projection_);
		if (corner_index == corner)
		{
			output.texcoord = vertex.texcoord_;
		}
	}

	if (single_sided && IsBackfacing(clip_positions[0], clip_positions[1], clip_positions[2]))
	{
		return output;
	}

	output.position = clip_positions[corner];
	return output;
}
