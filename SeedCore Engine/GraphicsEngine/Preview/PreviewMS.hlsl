#include <GraphicsEngine/Preview/Preview.hlsli>
#include <GraphicsEngine/Culling/BackfaceCulling.hlsli>

groupshared float4 clip_positions[64];

/**
* [EN]
* Preview.Draw.Mesh: one thread group per entry of Preview.Meshlets (an
* instance and one of its meshlets). Each vertex is skinned with
* Preview.Bones when the instance has a skin, then moved by the world
* transform; single-sided triangles that face away are collapsed, since the
* rasterizer itself culls nothing.
*
* ---------------------------------------------------------------------
*
* [JP]
* Preview.Draw.Mesh。Preview.Meshlets の 1 件（インスタンスとその meshlet の 1 つ）に
* つき 1 スレッドグループ。各頂点は、スキンのあるインスタンスなら Preview.Bones で
* スキニングしてから、ワールドの変換で動かす。ラスタライザーは何もカリングしないので、
* 片面で裏を向く三角形はここで潰す。
*/
[NumThreads(64, 1, 1)]
[OutputTopology("triangle")]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, out vertices PreviewOutput verts[64], out indices uint3 tris[124])
{
	StructuredBuffer<uint2> entries = ResourceDescriptorHeap[mesh_bindings.preview_meshlets_];
	uint2 entry = entries[gid];
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[mesh_bindings.preview_instances_];
	ModelStructuredBuffer instance = instances[entry.x];
	StructuredBuffer<ModelMeshlet> meshlets = ResourceDescriptorHeap[instance.geometry_.meshlet_buffer_index_];
	ModelMeshlet meshlet = meshlets[entry.y];
	SetMeshOutputCounts(meshlet.vertex_count_, meshlet.triangle_count_);

	StructuredBuffer<CompressedModelVertex> vertices = ResourceDescriptorHeap[instance.geometry_.vertex_buffer_index_];
	StructuredBuffer<uint> vertex_indices = ResourceDescriptorHeap[instance.geometry_.vertex_indices_buffer_index_];
	ByteAddressBuffer primitive_indices = ResourceDescriptorHeap[instance.geometry_.primitive_indices_buffer_index_];
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	if (gtid < meshlet.vertex_count_)
	{
		uint global_vertex_index = vertex_indices[meshlet.vertex_offset_ + gtid];
		ExpandedModelVertex vertex = DecodeModelVertex(vertices[global_vertex_index], instance);
		float3 local_position = vertex.position_;

		if (instance.skining_.skin_index_ != MODEL_INVALID)
		{
			StructuredBuffer<ModelBoneMatrix> bone_matrices = ResourceDescriptorHeap[mesh_bindings.preview_bones_];
			StructuredBuffer<CompressedModelSkin> skin_vertices = ResourceDescriptorHeap[instance.skining_.skin_vertex_buffer_index_];
			ExpandedModelSkin skin = DecodeSkinVertex(skin_vertices[global_vertex_index]);
			float4x4 skin_matrix =
				LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin.joints_.x]) * skin.weights_.x +
				LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin.joints_.y]) * skin.weights_.y +
				LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin.joints_.z]) * skin.weights_.z +
				LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin.joints_.w]) * skin.weights_.w;
			local_position = mul(float4(local_position, 1.0), skin_matrix).xyz;
		}

		float4 clip_position = mul(mul(float4(local_position, 1.0), instance.transform_.world_), scene.current_view_projection_);
		PreviewOutput output;
		output.position = clip_position;
		output.texcoord = vertex.texcoord_;
		output.instance_index = entry.x;
		verts[gtid] = output;
		clip_positions[gtid] = clip_position;
	}

	GroupMemoryBarrierWithGroupSync();

	for (uint triangle_index = gtid; triangle_index < meshlet.triangle_count_; triangle_index += 64)
	{
		uint byte_offset = meshlet.triangle_offset_ + triangle_index * 3;
		uint aligned_offset = byte_offset & ~3;
		uint shift = (byte_offset & 3) * 8;
		uint packed = primitive_indices.Load(aligned_offset) >> shift;
		if (shift > 8)
		{
			packed |= primitive_indices.Load(aligned_offset + 4) << (32 - shift);
		}

		uint index0 = packed & 0xFF;
		uint index1 = (packed >> 8) & 0xFF;
		uint index2 = (packed >> 16) & 0xFF;
		if (instance.shading_.double_sided_ == 0 && IsBackfacing(clip_positions[index0], clip_positions[index1], clip_positions[index2]))
		{
			tris[triangle_index] = uint3(0, 0, 0);
		}
		else
		{
			tris[triangle_index] = uint3(index0, index1, index2);
		}
	}
}
