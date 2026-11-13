#include <GraphicsEngine/Model/Opaque/Geometry.hlsli>
#include <GraphicsEngine/Culling/BackfaceCulling.hlsli>

/**
* [EN]
* Fallback for devices without mesh shaders: one DrawInstanced instance per
* entry of CullingDrawLists, 124 triangles' worth of vertices each. Vertices
* past the meshlet's triangle count are placed outside the clip volume. For a
* single-sided material every vertex transforms all three corners of its
* triangle so it can collapse back-facing triangles itself, matching the mesh
* path where the rasterizer culls nothing.
*/
void main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID, out GeometryOutput output, out GeometryPrimitive primitive)
{
	StructuredBuffer<uint2> draw_lists = ResourceDescriptorHeap[bindings.culling_draw_lists_];
	StructuredBuffer<uint> arguments = ResourceDescriptorHeap[bindings.culling_draw_arguments_];
	uint capacity;
	uint stride;
	draw_lists.GetDimensions(capacity, stride);
	uint base = dispatch_bindings.constants_ == CULLING_LATE_ARGUMENTS ? min(arguments[CULLING_EARLY_COUNT], capacity) : 0;
	uint2 entry = draw_lists[base + instance_id];

	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[entry.x];
	StructuredBuffer<ModelMeshlet> meshlets = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.meshlet_buffer_index_)];
	ModelMeshlet meshlet = meshlets[entry.y];

	uint triangle_index = vertex_id / 3;
	uint corner = vertex_id % 3;
	output = (GeometryOutput)0;
	output.position = float4(2.0, 2.0, 2.0, 1.0);
	primitive.triangle_in_meshlet_index = triangle_index;
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
	float2 texcoord = float2(0.0, 0.0);
	float3 object_position = float3(0.0, 0.0, 0.0);
	float3 object_normal = float3(0.0, 0.0, 1.0);
	float4 object_tangent = float4(1.0, 0.0, 0.0, 1.0);
	float3 world_position = float3(0.0, 0.0, 0.0);
	for (uint corner_index = 0; corner_index < 3; corner_index++)
	{
		if (!single_sided && corner_index != corner)
		{
			continue;
		}

		uint global_vertex_index = vertex_indices[meshlet.vertex_offset_ + ((packed >> (corner_index * 8)) & 0xFF)];
		ExpandedModelVertex vertex = DecodeModelVertex(vertices[global_vertex_index], instance);
		float3 local_position = vertex.position_;
		float3 local_normal = vertex.normal_;
		float3 local_tangent = vertex.tangent_.xyz;

		if (instance.morph_.morph_target_count_ != 0)
		{
			StructuredBuffer<uint> vertex_morph_source = ResourceDescriptorHeap[NonUniformResourceIndex(instance.morph_.vertex_morph_source_buffer_index_)];
			StructuredBuffer<float3> morph_deltas = ResourceDescriptorHeap[NonUniformResourceIndex(instance.morph_.morph_delta_buffer_index_)];
			StructuredBuffer<float> morph_weights = ResourceDescriptorHeap[bindings.model_morph_weights_];
			uint morph_vertex_index = vertex_morph_source[global_vertex_index] - instance.morph_.morph_vertex_offset_;
			for (uint target = 0; target < instance.morph_.morph_target_count_; target++)
			{
				local_position += morph_deltas[instance.morph_.morph_delta_offset_ + target * instance.morph_.morph_vertex_count_ + morph_vertex_index] * morph_weights[instance.morph_.morph_weight_offset_ + target];
			}
		}

		if (instance.skining_.skin_index_ != MODEL_INVALID)
		{
			StructuredBuffer<ModelBoneMatrix> bone_matrices = ResourceDescriptorHeap[bindings.model_bones_];
			StructuredBuffer<CompressedModelSkin> skin_vertices = ResourceDescriptorHeap[NonUniformResourceIndex(instance.skining_.skin_vertex_buffer_index_)];
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

		precise float4 corner_world_position = mul(float4(local_position, 1.0), instance.transform_.world_);
		corner_world_position.y -= fur_ratio * fur_ratio * instance.shading_.fur_length_ * 0.35;
		precise float4 corner_clip_position = mul(corner_world_position, scene.current_view_projection_);
		clip_positions[corner_index] = corner_clip_position;
		if (corner_index == corner)
		{
			texcoord = vertex.texcoord_;
			object_position = local_position;
			object_normal = normalize(local_normal);
			object_tangent = float4(normalize(local_tangent), vertex.tangent_.w);
			world_position = corner_world_position.xyz;
		}
	}

	if (single_sided && IsBackfacing(clip_positions[0], clip_positions[1], clip_positions[2]))
	{
		return;
	}

	output.position = clip_positions[corner];
	output.texcoord = texcoord;
	output.object_position = object_position;
	output.object_normal = object_normal;
	output.object_tangent = object_tangent;
	output.world_position = world_position;
	output.instance_index = entry.x;
	output.meshlet_index = entry.y;
}
