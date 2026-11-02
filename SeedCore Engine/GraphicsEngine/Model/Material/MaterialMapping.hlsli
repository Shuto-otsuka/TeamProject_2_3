#ifndef __MATERIAL_MAPPING_HLSL__
#define __MATERIAL_MAPPING_HLSL__

#include "../Model.hlsli"
#include "../../Shader/ShaderResources.hlsli"

struct MaterialMappingCoordinate
{
    float2 texcoord_x_;
    float2 texcoord_y_;
    float2 texcoord_z_;
    float3 weight_;
    float3 projection_normal_;
};

struct MaterialMappingSurface
{
	float2 texcoord_;
	float3 object_position_;
	float3 object_normal_;
	float4 object_tangent_;
	float3 world_position_;
	float3 world_normal_;
};

MaterialMappingCoordinate ResolveMaterialMapping(uint material_mapping, float tiling, float triplanar_sharpness, float2 texcoord, float3 object_position, float3 object_normal, float3 object_tangent, float3 object_bitangent, float3 object_scale, float3 world_position, float3 world_normal)
{
    MaterialMappingCoordinate coordinate;
    coordinate.texcoord_x_ = texcoord;
    coordinate.texcoord_y_ = texcoord;
    coordinate.texcoord_z_ = texcoord;
    coordinate.weight_ = float3(1.0, 0.0, 0.0);
    coordinate.projection_normal_ = world_normal;

    if (material_mapping == MATERIAL_MAPPING_TILE)
    {
        float tangent_length = length(object_tangent);
        float bitangent_length = length(object_bitangent);
        float tile_u = tangent_length > 1e-6 ? length(object_tangent * object_scale) / tangent_length : 1.0;
        float tile_v = bitangent_length > 1e-6 ? length(object_bitangent * object_scale) / bitangent_length : 1.0;
        coordinate.texcoord_x_ = texcoord * float2(tile_u, tile_v) * tiling;
    }
    else if (material_mapping == MATERIAL_MAPPING_TRIPLANAR_OBJECT || material_mapping == MATERIAL_MAPPING_TRIPLANAR_WORLD)
    {
        float3 position = world_position;
        float3 normal = world_normal;
        if (material_mapping == MATERIAL_MAPPING_TRIPLANAR_OBJECT)
        {
            position = object_position * object_scale;
            normal = normalize(object_normal / object_scale);
        }
        position *= tiling;

        coordinate.texcoord_x_ = position.zy;
        coordinate.texcoord_y_ = position.xz;
        coordinate.texcoord_z_ = position.xy;

        float3 weight = pow(abs(normal), triplanar_sharpness);
        coordinate.weight_ = weight / (weight.x + weight.y + weight.z);
        coordinate.projection_normal_ = normal;
    }

    return coordinate;
}

MaterialMappingSurface ReconstructMaterialMapping(ModelStructuredBuffer instance, uint meshlet_index, uint triangle_in_meshlet_index, float2 pixel_ndc, float4x4 view_projection, float3 camera_position)
{
	StructuredBuffer<CompressedModelVertex> vertices = ResourceDescriptorHeap[instance.geometry_.vertex_buffer_index_];
	StructuredBuffer<ModelMeshlet> meshlets = ResourceDescriptorHeap[instance.geometry_.meshlet_buffer_index_];
	StructuredBuffer<uint> vertex_indices = ResourceDescriptorHeap[instance.geometry_.vertex_indices_buffer_index_];
	ByteAddressBuffer primitive_indices = ResourceDescriptorHeap[instance.geometry_.primitive_indices_buffer_index_];

	ModelMeshlet meshlet = meshlets[meshlet_index];

	uint byte_offset = meshlet.triangle_offset_ + triangle_in_meshlet_index * 3;
	uint aligned_offset = byte_offset & ~3;
	uint shift = (byte_offset & 3) * 8;

	uint packed = primitive_indices.Load(aligned_offset) >> shift;
	if (shift > 8)
	{
		packed |= primitive_indices.Load(aligned_offset + 4) << (32 - shift);
	}

	uint global_index0 = vertex_indices[meshlet.vertex_offset_ + (packed & 0xFF)];
	uint global_index1 = vertex_indices[meshlet.vertex_offset_ + ((packed >> 8) & 0xFF)];
	uint global_index2 = vertex_indices[meshlet.vertex_offset_ + ((packed >> 16) & 0xFF)];

	ExpandedModelVertex vertex0 = DecodeModelVertex(vertices[global_index0], instance);
	ExpandedModelVertex vertex1 = DecodeModelVertex(vertices[global_index1], instance);
	ExpandedModelVertex vertex2 = DecodeModelVertex(vertices[global_index2], instance);

	float3 local_position0 = vertex0.position_;
	float3 local_position1 = vertex1.position_;
	float3 local_position2 = vertex2.position_;

	if (instance.morph_.morph_target_count_ != 0)
	{
		StructuredBuffer<uint> vertex_morph_source = ResourceDescriptorHeap[instance.morph_.vertex_morph_source_buffer_index_];
		StructuredBuffer<float3> morph_deltas = ResourceDescriptorHeap[instance.morph_.morph_delta_buffer_index_];
		StructuredBuffer<float> morph_weights = ResourceDescriptorHeap[shader_resource_indices.model_.morph_weight_index_];

		uint local_vertex_index0 = vertex_morph_source[global_index0] - instance.morph_.morph_vertex_offset_;
		uint local_vertex_index1 = vertex_morph_source[global_index1] - instance.morph_.morph_vertex_offset_;
		uint local_vertex_index2 = vertex_morph_source[global_index2] - instance.morph_.morph_vertex_offset_;

		for (uint target = 0; target < instance.morph_.morph_target_count_; ++target)
		{
			float weight = morph_weights[instance.morph_.morph_weight_offset_ + target];
			local_position0 += morph_deltas[instance.morph_.morph_delta_offset_ + target * instance.morph_.morph_vertex_count_ + local_vertex_index0] * weight;
			local_position1 += morph_deltas[instance.morph_.morph_delta_offset_ + target * instance.morph_.morph_vertex_count_ + local_vertex_index1] * weight;
			local_position2 += morph_deltas[instance.morph_.morph_delta_offset_ + target * instance.morph_.morph_vertex_count_ + local_vertex_index2] * weight;
		}
	}

	float3 local_normal0 = vertex0.normal_;
	float3 local_normal1 = vertex1.normal_;
	float3 local_normal2 = vertex2.normal_;
	float3 local_tangent0 = vertex0.tangent_.xyz;
	float3 local_tangent1 = vertex1.tangent_.xyz;
	float3 local_tangent2 = vertex2.tangent_.xyz;

	if (instance.skining_.skin_index_ != 0xFFFFFFFF)
	{
		StructuredBuffer<ModelBoneMatrix> bone_matrices = ResourceDescriptorHeap[shader_resource_indices.model_.bone_matrix_index_];
		StructuredBuffer<CompressedModelSkin> skin_vertices = ResourceDescriptorHeap[instance.skining_.skin_vertex_buffer_index_];

		ExpandedModelSkin skin0 = DecodeSkinVertex(skin_vertices[global_index0]);
		ExpandedModelSkin skin1 = DecodeSkinVertex(skin_vertices[global_index1]);
		ExpandedModelSkin skin2 = DecodeSkinVertex(skin_vertices[global_index2]);

		float4x4 skin_matrix0 =
			LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin0.joints_.x]) * skin0.weights_.x +
			LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin0.joints_.y]) * skin0.weights_.y +
			LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin0.joints_.z]) * skin0.weights_.z +
			LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin0.joints_.w]) * skin0.weights_.w;
		float4x4 skin_matrix1 =
			LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin1.joints_.x]) * skin1.weights_.x +
			LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin1.joints_.y]) * skin1.weights_.y +
			LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin1.joints_.z]) * skin1.weights_.z +
			LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin1.joints_.w]) * skin1.weights_.w;
		float4x4 skin_matrix2 =
			LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin2.joints_.x]) * skin2.weights_.x +
			LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin2.joints_.y]) * skin2.weights_.y +
			LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin2.joints_.z]) * skin2.weights_.z +
			LoadBoneMatrix(bone_matrices[instance.skining_.bone_offset_ + skin2.joints_.w]) * skin2.weights_.w;

		local_position0 = mul(float4(local_position0, 1.0), skin_matrix0).xyz;
		local_position1 = mul(float4(local_position1, 1.0), skin_matrix1).xyz;
		local_position2 = mul(float4(local_position2, 1.0), skin_matrix2).xyz;
		local_normal0 = normalize(mul(float4(local_normal0, 0.0), skin_matrix0).xyz);
		local_normal1 = normalize(mul(float4(local_normal1, 0.0), skin_matrix1).xyz);
		local_normal2 = normalize(mul(float4(local_normal2, 0.0), skin_matrix2).xyz);
		local_tangent0 = normalize(mul(float4(local_tangent0, 0.0), skin_matrix0).xyz);
		local_tangent1 = normalize(mul(float4(local_tangent1, 0.0), skin_matrix1).xyz);
		local_tangent2 = normalize(mul(float4(local_tangent2, 0.0), skin_matrix2).xyz);
	}

	float3 world_position0 = mul(float4(local_position0, 1.0), instance.transform_.world_).xyz;
	float3 world_position1 = mul(float4(local_position1, 1.0), instance.transform_.world_).xyz;
	float3 world_position2 = mul(float4(local_position2, 1.0), instance.transform_.world_).xyz;

	float4 clip0 = mul(float4(world_position0, 1.0), view_projection);
	float4 clip1 = mul(float4(world_position1, 1.0), view_projection);
	float4 clip2 = mul(float4(world_position2, 1.0), view_projection);

	float2 ndc0 = clip0.xy / clip0.w;
	float2 ndc1 = clip1.xy / clip1.w;
	float2 ndc2 = clip2.xy / clip2.w;

	float area = (ndc1.x - ndc0.x) * (ndc2.y - ndc0.y) - (ndc1.y - ndc0.y) * (ndc2.x - ndc0.x);
	float edge0 = (ndc2.x - ndc1.x) * (pixel_ndc.y - ndc1.y) - (ndc2.y - ndc1.y) * (pixel_ndc.x - ndc1.x);
	float edge1 = (ndc0.x - ndc2.x) * (pixel_ndc.y - ndc2.y) - (ndc0.y - ndc2.y) * (pixel_ndc.x - ndc2.x);
	float edge2 = (ndc1.x - ndc0.x) * (pixel_ndc.y - ndc0.y) - (ndc1.y - ndc0.y) * (pixel_ndc.x - ndc0.x);

	float perspective_w0 = (edge0 / area) / clip0.w;
	float perspective_w1 = (edge1 / area) / clip1.w;
	float perspective_w2 = (edge2 / area) / clip2.w;
	float perspective_sum = perspective_w0 + perspective_w1 + perspective_w2;
	perspective_w0 /= perspective_sum;
	perspective_w1 /= perspective_sum;
	perspective_w2 /= perspective_sum;

	MaterialMappingSurface surface;
	surface.texcoord_ = perspective_w0 * vertex0.texcoord_ + perspective_w1 * vertex1.texcoord_ + perspective_w2 * vertex2.texcoord_;
	surface.object_position_ = perspective_w0 * local_position0 + perspective_w1 * local_position1 + perspective_w2 * local_position2;
	surface.object_normal_ = normalize(perspective_w0 * local_normal0 + perspective_w1 * local_normal1 + perspective_w2 * local_normal2);
	surface.object_tangent_ = float4(normalize(perspective_w0 * local_tangent0 + perspective_w1 * local_tangent1 + perspective_w2 * local_tangent2), vertex0.tangent_.w);
	surface.world_position_ = perspective_w0 * world_position0 + perspective_w1 * world_position1 + perspective_w2 * world_position2;
	surface.world_normal_ = normalize(mul(float4(surface.object_normal_, 0.0), instance.transform_.inverse_transpose_world_).xyz);

	float3 geometric_face_normal = cross(world_position1 - world_position0, world_position2 - world_position0);
	if (dot(geometric_face_normal, surface.world_normal_) < 0.0)
	{
		geometric_face_normal = -geometric_face_normal;
	}
	if (dot(geometric_face_normal, camera_position - surface.world_position_) <= 0.0)
	{
		surface.object_normal_ = -surface.object_normal_;
		surface.world_normal_ = -surface.world_normal_;
	}

	return surface;
}

float4 SampleMaterialMappingGrad(Texture2D source_texture, SamplerState texture_sampler, MaterialMappingCoordinate coordinate)
{
    float2 ddx_x = ddx(coordinate.texcoord_x_);
    float2 ddy_x = ddy(coordinate.texcoord_x_);
    float2 ddx_y = ddx(coordinate.texcoord_y_);
    float2 ddy_y = ddy(coordinate.texcoord_y_);
    float2 ddx_z = ddx(coordinate.texcoord_z_);
    float2 ddy_z = ddy(coordinate.texcoord_z_);

    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    if (coordinate.weight_.x > 0.0)
    {
        color += source_texture.SampleGrad(texture_sampler, coordinate.texcoord_x_, ddx_x, ddy_x) * coordinate.weight_.x;
    }
    if (coordinate.weight_.y > 0.0)
    {
        color += source_texture.SampleGrad(texture_sampler, coordinate.texcoord_y_, ddx_y, ddy_y) * coordinate.weight_.y;
    }
    if (coordinate.weight_.z > 0.0)
    {
        color += source_texture.SampleGrad(texture_sampler, coordinate.texcoord_z_, ddx_z, ddy_z) * coordinate.weight_.z;
    }
    return color;
}

float3 SampleNormalMappingGrad(Texture2D normal_texture, SamplerState texture_sampler, MaterialMappingCoordinate coordinate)
{
    float2 ddx_x = ddx(coordinate.texcoord_x_);
    float2 ddy_x = ddy(coordinate.texcoord_x_);
    float2 ddx_y = ddx(coordinate.texcoord_y_);
    float2 ddy_y = ddy(coordinate.texcoord_y_);
    float2 ddx_z = ddx(coordinate.texcoord_z_);
    float2 ddy_z = ddy(coordinate.texcoord_z_);

    float3 normal = coordinate.projection_normal_;
    float3 result = float3(0.0, 0.0, 0.0);
    if (coordinate.weight_.x > 0.0)
    {
        float3 tangent_normal = normal_texture.SampleGrad(texture_sampler, coordinate.texcoord_x_, ddx_x, ddy_x).xyz * 2.0 - 1.0;
        float3 blended = float3(tangent_normal.xy + normal.zy, abs(tangent_normal.z) * normal.x);
        result += blended.zyx * coordinate.weight_.x;
    }
    if (coordinate.weight_.y > 0.0)
    {
        float3 tangent_normal = normal_texture.SampleGrad(texture_sampler, coordinate.texcoord_y_, ddx_y, ddy_y).xyz * 2.0 - 1.0;
        float3 blended = float3(tangent_normal.xy + normal.xz, abs(tangent_normal.z) * normal.y);
        result += blended.xzy * coordinate.weight_.y;
    }
    if (coordinate.weight_.z > 0.0)
    {
        float3 tangent_normal = normal_texture.SampleGrad(texture_sampler, coordinate.texcoord_z_, ddx_z, ddy_z).xyz * 2.0 - 1.0;
        float3 blended = float3(tangent_normal.xy + normal.xy, abs(tangent_normal.z) * normal.z);
        result += blended.xyz * coordinate.weight_.z;
    }
    return normalize(result);
}

float4 SampleMaterialMappingLevel(Texture2D source_texture, SamplerState texture_sampler, MaterialMappingCoordinate coordinate, float level)
{
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    if (coordinate.weight_.x > 0.0)
    {
        color += source_texture.SampleLevel(texture_sampler, coordinate.texcoord_x_, level) * coordinate.weight_.x;
    }
    if (coordinate.weight_.y > 0.0)
    {
        color += source_texture.SampleLevel(texture_sampler, coordinate.texcoord_y_, level) * coordinate.weight_.y;
    }
    if (coordinate.weight_.z > 0.0)
    {
        color += source_texture.SampleLevel(texture_sampler, coordinate.texcoord_z_, level) * coordinate.weight_.z;
    }
    return color;
}

float3 SampleNormalMappingLevel(Texture2D normal_texture, SamplerState texture_sampler, MaterialMappingCoordinate coordinate, float level)
{
    float3 normal = coordinate.projection_normal_;
    float3 result = float3(0.0, 0.0, 0.0);
    if (coordinate.weight_.x > 0.0)
    {
        float3 tangent_normal = normal_texture.SampleLevel(texture_sampler, coordinate.texcoord_x_, level).xyz * 2.0 - 1.0;
        float3 blended = float3(tangent_normal.xy + normal.zy, abs(tangent_normal.z) * normal.x);
        result += blended.zyx * coordinate.weight_.x;
    }
    if (coordinate.weight_.y > 0.0)
    {
        float3 tangent_normal = normal_texture.SampleLevel(texture_sampler, coordinate.texcoord_y_, level).xyz * 2.0 - 1.0;
        float3 blended = float3(tangent_normal.xy + normal.xz, abs(tangent_normal.z) * normal.y);
        result += blended.xzy * coordinate.weight_.y;
    }
    if (coordinate.weight_.z > 0.0)
    {
        float3 tangent_normal = normal_texture.SampleLevel(texture_sampler, coordinate.texcoord_z_, level).xyz * 2.0 - 1.0;
        float3 blended = float3(tangent_normal.xy + normal.xy, abs(tangent_normal.z) * normal.z);
        result += blended.xyz * coordinate.weight_.z;
    }
    return normalize(result);
}

#endif // __MATERIAL_MAPPING_HLSL__