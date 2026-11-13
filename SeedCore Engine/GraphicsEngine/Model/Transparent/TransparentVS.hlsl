#include <GraphicsEngine/Model/Transparent/Transparent.hlsli>
#include <GraphicsEngine/Culling/BackfaceCulling.hlsli>

/**
* [EN]
* Fallback for devices without mesh shaders: one DrawInstanced instance per
* entry of Culling.TransparentLists, 124 triangles' worth of vertices each.
* Vertices past the meshlet's triangle count are placed outside the clip
* volume. For a single-sided material every vertex transforms all three
* corners of its triangle so it can collapse back-facing triangles itself,
* matching the mesh path where the rasterizer culls nothing.
*
* ---------------------------------------------------------------------
*
* [JP]
* メッシュシェーダーが使えないデバイス向けの代わり。Culling.TransparentLists の
* 1 件につき DrawInstanced の 1 インスタンスで、三角形 124 個分の頂点を描く。
* メッシュレットの三角形数を超えた頂点はクリップ空間の外へ置く。片面の
* マテリアルでは、各頂点が三角形の 3 つの角をすべて変換し、裏を向く三角形を
* 自分で潰す。ラスタライザーが何もカリングしないメッシュの経路と合わせるため。
*/
void main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID, out TransparentOutput output)
{
	StructuredBuffer<uint2> lists = ResourceDescriptorHeap[bindings.culling_transparent_lists_];
	uint2 entry = lists[instance_id];

	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[entry.x];
	StructuredBuffer<ModelMeshlet> meshlets = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.meshlet_buffer_index_)];
	ModelMeshlet meshlet = meshlets[entry.y];

	uint triangle_index = vertex_id / 3;
	uint corner = vertex_id % 3;
	output = (TransparentOutput)0;
	output.position = float4(2.0, 2.0, 2.0, 1.0);
	output.instance_index = entry.x;
	if (triangle_index >= meshlet.triangle_count_)
	{
		return;
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
		float3 previous_local_position = vertex.position_;
		float3 local_normal = vertex.normal_;
		float3 local_tangent = vertex.tangent_.xyz;

		if (instance.morph_.morph_target_count_ != 0)
		{
			StructuredBuffer<uint> vertex_morph_source = ResourceDescriptorHeap[NonUniformResourceIndex(instance.morph_.vertex_morph_source_buffer_index_)];
			StructuredBuffer<float3> morph_deltas = ResourceDescriptorHeap[NonUniformResourceIndex(instance.morph_.morph_delta_buffer_index_)];
			StructuredBuffer<float> morph_weights = ResourceDescriptorHeap[bindings.model_morph_weights_];
			uint weight_count;
			uint weight_stride;
			morph_weights.GetDimensions(weight_count, weight_stride);
			uint previous_weight_base = weight_count / 2;
			uint morph_vertex_index = vertex_morph_source[global_vertex_index] - instance.morph_.morph_vertex_offset_;
			for (uint target = 0; target < instance.morph_.morph_target_count_; target++)
			{
				float3 morph_delta = morph_deltas[instance.morph_.morph_delta_offset_ + target * instance.morph_.morph_vertex_count_ + morph_vertex_index];
				local_position += morph_delta * morph_weights[instance.morph_.morph_weight_offset_ + target];
				previous_local_position += morph_delta * morph_weights[previous_weight_base + instance.morph_.morph_weight_offset_ + target];
			}
		}

		if (instance.skining_.skin_index_ != MODEL_INVALID)
		{
			StructuredBuffer<ModelBoneMatrix> bone_matrices = ResourceDescriptorHeap[bindings.model_bones_];
			StructuredBuffer<CompressedModelSkin> skin_vertices = ResourceDescriptorHeap[NonUniformResourceIndex(instance.skining_.skin_vertex_buffer_index_)];
			uint bone_count;
			uint bone_stride;
			bone_matrices.GetDimensions(bone_count, bone_stride);
			uint bone_base = instance.skining_.bone_offset_;
			uint previous_bone_base = bone_count / 2 + instance.skining_.bone_offset_;
			ExpandedModelSkin skin = DecodeSkinVertex(skin_vertices[global_vertex_index]);
			float4x4 skin_matrix =
				LoadBoneMatrix(bone_matrices[bone_base + skin.joints_.x]) * skin.weights_.x +
				LoadBoneMatrix(bone_matrices[bone_base + skin.joints_.y]) * skin.weights_.y +
				LoadBoneMatrix(bone_matrices[bone_base + skin.joints_.z]) * skin.weights_.z +
				LoadBoneMatrix(bone_matrices[bone_base + skin.joints_.w]) * skin.weights_.w;
			float4x4 previous_skin_matrix =
				LoadBoneMatrix(bone_matrices[previous_bone_base + skin.joints_.x]) * skin.weights_.x +
				LoadBoneMatrix(bone_matrices[previous_bone_base + skin.joints_.y]) * skin.weights_.y +
				LoadBoneMatrix(bone_matrices[previous_bone_base + skin.joints_.z]) * skin.weights_.z +
				LoadBoneMatrix(bone_matrices[previous_bone_base + skin.joints_.w]) * skin.weights_.w;
			local_position = mul(float4(local_position, 1.0), skin_matrix).xyz;
			previous_local_position = mul(float4(previous_local_position, 1.0), previous_skin_matrix).xyz;
			local_normal = normalize(mul(float4(local_normal, 0.0), skin_matrix).xyz);
			local_tangent = normalize(mul(float4(local_tangent, 0.0), skin_matrix).xyz);
		}

		float3 world_position = mul(float4(local_position, 1.0), instance.transform_.world_).xyz;
		clip_positions[corner_index] = mul(float4(world_position, 1.0), scene.current_view_projection_);
		if (corner_index == corner)
		{
			float3 previous_world_position = mul(float4(previous_local_position, 1.0), instance.transform_.previous_world_).xyz;
			output.world_position = world_position;
			output.object_position = local_position;
			output.normal = normalize(mul(float4(local_normal, 0.0), instance.transform_.inverse_transpose_world_).xyz);
			output.object_normal = local_normal;
			output.tangent = float4(normalize(mul(float4(local_tangent, 0.0), instance.transform_.world_).xyz), vertex.tangent_.w);
			output.object_tangent = local_tangent;
			output.texcoord = vertex.texcoord_;
			output.current_clip = clip_positions[corner_index];
			output.previous_clip = mul(float4(previous_world_position, 1.0), scene.previous_view_projection_);
		}
	}

	if (single_sided && IsBackfacing(clip_positions[0], clip_positions[1], clip_positions[2]))
	{
		return;
	}

	output.position = clip_positions[corner];
}
