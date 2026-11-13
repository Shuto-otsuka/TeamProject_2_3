#include <GraphicsEngine/Model/Transparent/Transparent.hlsli>
#include <GraphicsEngine/Culling/BackfaceCulling.hlsli>

groupshared float4 clip_positions[64];

/**
* [EN]
* One thread group per entry of Culling.TransparentLists. Each vertex applies
* morph targets, then linear blend skinning, then the world transform, for
* this frame and, with the second half of ModelBones / ModelMorphWeights, for
* the previous one, and hands the pixel shader the world and object space
* position, normal and tangent it shades with. Single-sided triangles that
* face away are collapsed, since the rasterizer itself culls nothing.
*
* ---------------------------------------------------------------------
*
* [JP]
* Culling.TransparentLists の 1 件につき 1 スレッドグループ。各頂点はモーフ、
* 次にスキン、次にワールド変換を、このフレームの分と、ModelBones /
* ModelMorphWeights の後ろ半分で前のフレームの分を求め、ピクセルシェーダーが
* 陰影付けに使うワールドとオブジェクト空間の位置・法線・タンジェントを渡す。
* ラスタライザー自身は何もカリングしないので、片面で裏を向く三角形は潰す。
*/
[NumThreads(64, 1, 1)]
[OutputTopology("triangle")]
void main(uint gtid : SV_GroupThreadID, uint3 gid : SV_GroupID, out vertices TransparentOutput verts[64], out indices uint3 tris[124])
{
	StructuredBuffer<uint2> lists = ResourceDescriptorHeap[bindings.culling_transparent_lists_];
	StructuredBuffer<uint> arguments = ResourceDescriptorHeap[bindings.culling_transparent_arguments_];
	uint capacity;
	uint stride;
	lists.GetDimensions(capacity, stride);
	uint count = min(arguments[CULLING_TRANSPARENT_COUNT], capacity);
	uint entry_index = gid.y * 1024 + gid.x;
	if (entry_index >= count)
	{
		return;
	}

	uint2 entry = lists[entry_index];
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
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
		float3 previous_local_position = vertex.position_;
		float3 local_normal = vertex.normal_;
		float3 local_tangent = vertex.tangent_.xyz;

		if (instance.morph_.morph_target_count_ != 0)
		{
			StructuredBuffer<uint> vertex_morph_source = ResourceDescriptorHeap[instance.morph_.vertex_morph_source_buffer_index_];
			StructuredBuffer<float3> morph_deltas = ResourceDescriptorHeap[instance.morph_.morph_delta_buffer_index_];
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
			StructuredBuffer<CompressedModelSkin> skin_vertices = ResourceDescriptorHeap[instance.skining_.skin_vertex_buffer_index_];
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
		float3 previous_world_position = mul(float4(previous_local_position, 1.0), instance.transform_.previous_world_).xyz;
		float4 clip_position = mul(float4(world_position, 1.0), scene.current_view_projection_);

		TransparentOutput output;
		output.position = clip_position;
		output.world_position = world_position;
		output.object_position = local_position;
		output.normal = normalize(mul(float4(local_normal, 0.0), instance.transform_.inverse_transpose_world_).xyz);
		output.object_normal = local_normal;
		output.tangent = float4(normalize(mul(float4(local_tangent, 0.0), instance.transform_.world_).xyz), vertex.tangent_.w);
		output.object_tangent = local_tangent;
		output.texcoord = vertex.texcoord_;
		output.current_clip = clip_position;
		output.previous_clip = mul(float4(previous_world_position, 1.0), scene.previous_view_projection_);
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
