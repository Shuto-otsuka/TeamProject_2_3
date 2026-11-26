#ifndef __MODEL_HLSL__
#define __MODEL_HLSL__

#include <GraphicsEngine/Shader/Normal.hlsli>

#define SHADING_MODEL_PBR 0
#define SHADING_MODEL_UNLIT 1
#define SHADING_MODEL_PHONG 2
#define SHADING_MODEL_TOON 3
#define SHADING_MODEL_LAMBERT 4
#define SHADING_MODEL_FLAT 5
#define SHADING_MODEL_FUR 6

#define MATERIAL_MAPPING_STRETCH 0
#define MATERIAL_MAPPING_TILE 1
#define MATERIAL_MAPPING_TRIPLANAR_OBJECT 2
#define MATERIAL_MAPPING_TRIPLANAR_WORLD 3

struct ModelTransform
{
	row_major float4x4 world_;
	row_major float4x4 inverse_transpose_world_;
	row_major float4x4 previous_world_;
};

struct ModelGeometry
{
	uint vertex_buffer_index_;
	uint meshlet_buffer_index_;
	uint meshlet_bound_buffer_index_;
	uint vertex_indices_buffer_index_;
	uint primitive_indices_buffer_index_;
	uint meshlet_offset_;
	uint meshlet_count_;
	uint layer_;
};

struct ModelStreaming
{
	float3 position_min_;
	float texcoord_min_u_;

	float3 position_extent_;
	float texcoord_min_v_;

	float2 texcoord_extent_;
	float lod_error_;
	uint lod_level_;
};

struct ModelShading
{
	uint shading_model_;

	float fur_length_;

	float fur_density_;

	uint fur_shell_count_;

	uint double_sided_;

	uint blend_;

	uint selected_;

	uint material_mapping_;

	float tiling_;

	float triplanar_sharpness_;

	uint actor_;

	uint model_shading_padding_0_;
};

struct ModelSkining
{
	uint skin_index_;
	uint bone_offset_;
	uint skin_vertex_buffer_index_;
	uint model_skining_padding_0_;
};

struct ModelMorphing
{
	uint morph_delta_buffer_index_;
	uint vertex_morph_source_buffer_index_;
	uint morph_delta_offset_;
	uint morph_vertex_offset_;

	uint morph_vertex_count_;
	uint morph_target_count_;
	uint morph_weight_offset_;
	uint model_morph_padding_0_;
};

struct ModelTexture
{
	float4 base_color_;
	float metallic_;
	float roughness_;
	float alpha_cutoff_;
	float ior_;

	float3 emissive_;
	float emissive_strength_;

	uint base_color_texture_index_;
	uint normal_texture_index_;
	uint metallic_roughness_texture_index_;
	uint emissive_texture_index_;

	uint occlusion_texture_index_;
	uint3 model_texture_padding_0_;
};

struct ModelExtension
{
	float specular_factor_;
	float3 specular_color_;
	float clearcoat_factor_;
	float clearcoat_roughness_;
	float anisotropy_;
	float anisotropy_rotation_;
	float transmission_factor_;
	float volume_thickness_factor_;
	float volume_attenuation_distance_;
	float unlit_;
	float3 volume_attenuation_color_;
	float sheen_roughness_;
	float3 sheen_color_;
	float iridescence_factor_;
	float iridescence_ior_;
	float iridescence_thickness_;
	uint specular_texture_index_;
	uint specular_color_texture_index_;
	uint clearcoat_texture_index_;
	uint clearcoat_roughness_texture_index_;
	uint clearcoat_normal_texture_index_;
	uint transmission_texture_index_;
	uint thickness_texture_index_;
	uint sheen_color_texture_index_;
	uint sheen_roughness_texture_index_;
	uint iridescence_texture_index_;
	uint iridescence_thickness_texture_index_;
	uint anisotropy_texture_index_;
	uint2 model_extension_padding_0_;
};

struct ModelStructuredBuffer
{
	ModelTransform transform_;
	ModelGeometry geometry_;
	ModelStreaming streaming_;
	ModelShading shading_;
	ModelSkining skining_;
	ModelMorphing morph_;
	ModelTexture texture_;
	ModelExtension extension_;
};

struct ExpandedModelVertex
{
	float3 position_;
	float3 normal_;
	float4 tangent_;
	float2 texcoord_;
};

struct CompressedModelVertex
{
	uint position_xy_;
	uint position_z_texu_;
	uint texv_tangent_;
	uint normal_;
};

struct CompressedModelSkin
{
	uint joints_xy_;
	uint joints_zw_;
	uint weights_;
};

struct ExpandedModelSkin
{
	uint4 joints_;
	float4 weights_;
};

struct ModelMeshlet
{
	uint vertex_offset_;
	uint triangle_offset_;
	uint vertex_count_;
	uint triangle_count_;
};

struct ModelMeshletBound
{
	float3 center_;
	float radius_;
	float3 cone_axis_;
	float cone_cutoff_;
};

struct ModelBoneMatrix
{
	float4 row0_;
	float4 row1_;
	float4 row2_;
	float4 row3_;
};

ExpandedModelVertex DecodeModelVertex(CompressedModelVertex packed, ModelStructuredBuffer instance)
{
	ExpandedModelVertex vertex;
	float3 position01 = float3(packed.position_xy_ & 0xFFFF, packed.position_xy_ >> 16, packed.position_z_texu_ & 0xFFFF) / 65535.0;
	vertex.position_ = instance.streaming_.position_min_ + position01 * instance.streaming_.position_extent_;
	float2 texcoord01 = float2(packed.position_z_texu_ >> 16, packed.texv_tangent_ & 0xFFFF) / 65535.0;
	vertex.texcoord_ = float2(instance.streaming_.texcoord_min_u_, instance.streaming_.texcoord_min_v_) + texcoord01 * instance.streaming_.texcoord_extent_;
	vertex.normal_ = OctNormalDecode(float2(packed.normal_ & 0xFFFF, packed.normal_ >> 16) / 65535.0);
	uint tangent_bits = packed.texv_tangent_ >> 16;
	float2 tangent_oct = float2(tangent_bits & 0xFF, (tangent_bits >> 8) & 0x7F) / float2(255.0, 127.0);
	float tangent_sign = (tangent_bits >> 15) ? 1.0 : -1.0;
	vertex.tangent_ = float4(OctNormalDecode(tangent_oct), tangent_sign);
	return vertex;
}

ExpandedModelSkin DecodeSkinVertex(CompressedModelSkin packed)
{
	ExpandedModelSkin skin;
	skin.joints_ = uint4(packed.joints_xy_ & 0xFFFF, packed.joints_xy_ >> 16, packed.joints_zw_ & 0xFFFF, packed.joints_zw_ >> 16);
	skin.weights_ = float4(packed.weights_ & 0xFF, (packed.weights_ >> 8) & 0xFF, (packed.weights_ >> 16) & 0xFF, packed.weights_ >> 24) / 255.0;
	skin.weights_ /= max(dot(skin.weights_, 1.0), 1e-6);
	return skin;
}

float4x4 LoadBoneMatrix(ModelBoneMatrix bone)
{
	return float4x4(bone.row0_, bone.row1_, bone.row2_, bone.row3_);
}

uint2 PackVisibilityID(uint instance_index, uint meshlet_index, uint triangle_in_meshlet_index)
{
	return uint2(instance_index + 1, (meshlet_index << 7) | (triangle_in_meshlet_index & 0x7F));
}

void UnpackVisibilityID(uint2 packed, out uint instance_index, out uint meshlet_index, out uint triangle_in_meshlet_index)
{
	instance_index = packed.x - 1;
	meshlet_index = packed.y >> 7;
	triangle_in_meshlet_index = packed.y & 0x7F;
}

#define MODEL_UNIT_ACTIVE 1
#define MODEL_UNIT_SELECTED 2

#define MODEL_CLUSTER_RESIDENT 1
#define MODEL_CLUSTER_STANDALONE_VERTICES 2

#define MODEL_INVALID 0xFFFFFFFF
#define MODEL_MESHLETS_PER_INSTANCE 32
#define MODEL_FLOAT_MAX 3.402823466e+38

#define MODEL_COUNT_OPAQUE 0
#define MODEL_COUNT_TRANSPARENT 1
#define MODEL_COUNT_OVERFLOW 3

struct ModelUnitStructuredBuffer
{
    row_major float4x4 world_;
    row_major float4x4 previous_world_;
    uint mesh_offset_;
    uint mesh_count_;
    uint material_offset_;
    uint flags_;
    uint bone_offset_;
    uint morph_weight_offset_;
    uint model_unit_structured_buffer_padding_0_;
    uint model_unit_structured_buffer_padding_1_;
};

struct ModelMeshStructuredBuffer
{
    float3 position_min_;
    float texcoord_min_u_;

    float3 position_extent_;
    float texcoord_min_v_;

    float2 texcoord_extent_;
    float texcoord_density_;
    uint surface_index_;

    uint cluster_offset_;
    uint cluster_count_;
    uint placement_offset_;
    uint placement_count_;

    uint skin_index_;
    uint skin_bone_offset_;
    uint skin_vertex_buffer_index_;
    uint morph_delta_buffer_index_;

    uint vertex_morph_source_buffer_index_;
    uint morph_delta_offset_;
    uint vertex_offset_;
    uint vertex_count_;

    uint morph_target_count_;
    uint morph_weight_offset_;
    uint model_mesh_structured_buffer_padding_0_;
    uint model_mesh_structured_buffer_padding_1_;
};

struct ModelClusterStructuredBuffer
{
    uint meshlet_offset_;
    uint meshlet_count_;
    float lod_error_;
    uint flags_;

    uint vertex_buffer_index_;
    uint meshlet_buffer_index_;
    uint meshlet_bound_buffer_index_;
    uint vertex_indices_buffer_index_;

    uint primitive_indices_buffer_index_;
    uint model_cluster_structured_buffer_padding_0_;
    uint model_cluster_structured_buffer_padding_1_;
    uint model_cluster_structured_buffer_padding_2_;
};

struct ModelPlacementStructuredBuffer
{
    row_major float4x4 placement_;
};

struct ModelMaterialStructuredBuffer
{
    ModelShading shading_;
    ModelTexture texture_;
    ModelExtension extension_;
};

struct ModelUnitUpdate
{
    uint index_;
    uint3 model_unit_update_padding_0_;
    ModelUnitStructuredBuffer data_;
};

struct ModelMeshUpdate
{
    uint index_;
    uint3 model_mesh_update_padding_0_;
    ModelMeshStructuredBuffer data_;
};

struct ModelClusterUpdate
{
    uint index_;
    uint3 model_cluster_update_padding_0_;
    ModelClusterStructuredBuffer data_;
};

struct ModelPlacementUpdate
{
    uint index_;
    uint3 model_placement_update_padding_0_;
    ModelPlacementStructuredBuffer data_;
};

struct ModelMaterialUpdate
{
    uint index_;
    uint3 model_material_update_padding_0_;
    ModelMaterialStructuredBuffer data_;
};

struct ModelRaytracingStructuredBuffer
{
    uint2 bottom_level_;
    uint vertex_buffer_index_;
    uint index_buffer_index_;

    uint surface_buffer_index_;
    uint model_raytracing_structured_buffer_padding_0_;
    float2 texcoord_min_;

    float2 texcoord_extent_;
    uint model_raytracing_structured_buffer_padding_1_;
    uint model_raytracing_structured_buffer_padding_2_;
};

struct ModelRaytracingUpdate
{
    uint index_;
    uint3 model_raytracing_update_padding_0_;
    ModelRaytracingStructuredBuffer data_;
};

struct ModelDeformStructuredBuffer
{
    uint unit_index_;
    uint sub_mesh_index_;
    uint vertex_offset_;
    uint vertex_count_;

    uint output_offset_;
    uint morph_delta_offset_;
    uint morph_target_count_;
    uint position_buffer_index_;

    uint skin_vertex_buffer_index_;
    uint morph_delta_buffer_index_;
    uint2 bottom_level_;
};

struct ModelExpandConstantBuffer
{
    float4 planes_[18];
    float4 cameras_[3];

    uint view_count_;
    uint unit_count_;
    uint transparent_base_;
    uint capacity_;

    uint softbody_opaque_count_;
    uint softbody_transparent_count_;
    uint2 model_expand_constant_buffer_padding_0_;
};

#endif
