#include <GraphicsEngine/Model/Opaque/Geometry.hlsli>
#include <GraphicsEngine/Culling/BackfaceCulling.hlsli>

groupshared float4 clip_positions[64];

/**
* [EN]
* One thread group per (instance, meshlet) entry GeometryAS kept, read from
* its payload. Each vertex applies morph targets, then linear blend skinning,
* then the world transform; a fur shell layer is pushed out along the skinned
* normal and droops towards world down as it rises. Single-sided triangles
* that face away are collapsed, since the rasterizer itself culls nothing.
*/
[NumThreads(64, 1, 1)]
[OutputTopology("triangle")]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, in payload GeometryPayload payload, out vertices GeometryOutput verts[64], out indices uint3 tris[124], out primitives GeometryPrimitive prims[124])
{
	uint2 entry = payload.entries_[gid];
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
		float3 local_normal = vertex.normal_;
		float3 local_tangent = vertex.tangent_.xyz;

		if (instance.morph_.morph_target_count_ != 0)
		{
			StructuredBuffer<uint> vertex_morph_source = ResourceDescriptorHeap[instance.morph_.vertex_morph_source_buffer_index_];
			StructuredBuffer<float3> morph_deltas = ResourceDescriptorHeap[instance.morph_.morph_delta_buffer_index_];
			StructuredBuffer<float> morphs = ResourceDescriptorHeap[bindings.model_morphs_];
			uint morph_vertex_index = vertex_morph_source[global_vertex_index] - instance.morph_.morph_vertex_offset_;
			for (uint target = 0; target < instance.morph_.morph_target_count_; target++)
			{
				local_position += morph_deltas[instance.morph_.morph_delta_offset_ + target * instance.morph_.morph_vertex_count_ + morph_vertex_index] * morphs[instance.morph_.morph_weight_offset_ + target];
			}
		}

		if (instance.skining_.skin_index_ != MODEL_INVALID)
		{
			StructuredBuffer<ModelBoneMatrix> bone_matrices = ResourceDescriptorHeap[bindings.model_bones_];
			StructuredBuffer<CompressedModelSkin> skin_vertices = ResourceDescriptorHeap[instance.skining_.skin_vertex_buffer_index_];
			ExpandedModelSkin skin = DecodeSkinVertex(skin_vertices[global_vertex_index]);
			float4x4 skin_matrix =
				LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin.joints_.x]) * skin.weights_.x +
				LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin.joints_.y]) * skin.weights_.y +
				LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin.joints_.z]) * skin.weights_.z +
				LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin.joints_.w]) * skin.weights_.w;
			local_position = mul(float4(local_position, 1.0), skin_matrix).xyz;
			local_normal = mul(float4(local_normal, 0.0), skin_matrix).xyz;
			local_tangent = mul(float4(local_tangent, 0.0), skin_matrix).xyz;
		}

		float fur_ratio = 0.0;
		if (instance.geometry_.layer_ != 0)
		{
			fur_ratio = (float)instance.geometry_.layer_ / (float)instance.shading_.fur_shell_count_;
			local_position += normalize(local_normal) * pow(fur_ratio, 1.35) * instance.shading_.fur_length_;
		}

		precise float4 world_position = mul(float4(local_position, 1.0), instance.transform_.world_);
		world_position.y -= fur_ratio * fur_ratio * instance.shading_.fur_length_ * 0.35;
		precise float4 clip_position = mul(world_position, scene.current_view_projection_);

		GeometryOutput output;
		output.position = clip_position;
		output.texcoord = vertex.texcoord_;
		output.object_position = local_position;
		output.object_normal = normalize(local_normal);
		output.object_tangent = float4(normalize(local_tangent), vertex.tangent_.w);
		output.world_position = world_position.xyz;
		output.instance_index = entry.x;
		output.meshlet_index = entry.y;
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
		prims[triangle_index].triangle_in_meshlet_index = triangle_index;
	}
}
