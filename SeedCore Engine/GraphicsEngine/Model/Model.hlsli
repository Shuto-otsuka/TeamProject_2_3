#ifndef __MODEL_HLSL__
#define __MODEL_HLSL__

#include <GraphicsEngine/Shader/Normal.hlsli>

// Mirrors the C++ ShadingModel enum (Model/Crister/CristerMaterial.h). Read from
// ModelShading::shading_model_ and switched on by
// Opaque/DeferredLightingPS.hlsl's direct-light dispatch.
#define SHADING_MODEL_PBR 0
#define SHADING_MODEL_UNLIT 1
#define SHADING_MODEL_PHONG 2
#define SHADING_MODEL_TOON 3
#define SHADING_MODEL_LAMBERT 4
#define SHADING_MODEL_FLAT 5
#define SHADING_MODEL_FUR 6

// Mirrors the C++ MaterialMapping enum. Read from ModelShading::material_mapping_.
#define MATERIAL_MAPPING_STRETCH 0
#define MATERIAL_MAPPING_TILE 1
#define MATERIAL_MAPPING_TRIPLANAR_OBJECT 2
#define MATERIAL_MAPPING_TRIPLANAR_WORLD 3

/**
* [EN]
* previous_world_ is this instance's own world matrix as of the previous
* frame - used for velocity (MaterialResolveCS.hlsl/TransparentMS.hlsl)
* so moving objects get correct motion vectors instead of only reflecting
* camera motion. Per-bone skinning and morph deformation come from the
* previous-frame bone palette / morph weights instead
* (ModelShaderResourceIndices::previous_bone_matrix_index_/previous_morph_weight_index_).
*/
struct ModelTransform                     // 12 rows
{
	row_major float4x4 world_;
	row_major float4x4 inverse_transpose_world_;
	row_major float4x4 previous_world_;
};

/**
* [EN]
* Bindless SRVs of this instance's geometry buffers (compressed vertices,
* meshlets, meshlet bounds, meshlet-local vertex indices, packed primitive
* indices), plus meshlet_offset_/meshlet_count_ - this instance's range
* into the shared meshlet buffers.
*/
struct ModelGeometry                      // 2 rows
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

/**
* [EN]
* Dequantisation AABB for CompressedModelVertex (per Crister), plus the
* screen-space error in pixels of this instance's cluster as seen from the
* closest view, and the cluster's level in its LOD chain (0 = most detailed).
*/
struct ModelStreaming                     // 3 rows
{
	float3 position_min_;
	float texcoord_min_u_;

	float3 position_extent_;
	float texcoord_min_v_;

	float2 texcoord_extent_;
	float lod_error_;
	uint lod_level_;
};

/**
* [EN]
* Per-instance shading parameters copied from the instance's material and
* its actor. shading_model_ selects the lighting response
* DeferredLightingPS.hlsl evaluates this instance with;
* fur_length_/fur_density_/fur_shell_count_ are ShadingModel::Fur tuning.
* double_sided_/blend_ route the instance to the culling lists and passes
* that match its faces and alpha mode, and selected_ marks an actor the
* editor has selected. material_mapping_/tiling_/triplanar_sharpness_
* decide how the material's textures are placed (MaterialMapping.hlsli),
* and actor_ is the entity index of the actor the instance belongs to,
* which the outline looks its style up by (OutlineCS.hlsl).
*/
struct ModelShading                       // 3 rows
{
	/// Lighting response the deferred lighting pass evaluates this instance with (SHADING_MODEL_*).
	uint shading_model_;

	/// Fur shell height in metres (SHADING_MODEL_FUR).
	float fur_length_;

	/// Per-strand hash-noise density of the fur (SHADING_MODEL_FUR).
	float fur_density_;

	/// Number of shell layers drawn above the base surface (SHADING_MODEL_FUR).
	uint fur_shell_count_;

	/// 1 when the material draws both faces; culling puts the instance in the double-sided list and skips back-face rejection.
	uint double_sided_;

	/// 1 when the material's alpha mode is BLEND; the instance is drawn by the transparent (OIT) pass instead of the opaque ones.
	uint blend_;

	/// 1 when the instance's actor is selected in the editor.
	uint selected_;

	/// How the material's textures are placed (MATERIAL_MAPPING_*).
	uint material_mapping_;

	/// Pattern repetitions per unit of scaled size; unused by MATERIAL_MAPPING_STRETCH.
	float tiling_;

	/// Exponent applied to the normal before it weights the three Triplanar projections.
	float triplanar_sharpness_;

	/// Entity index of the actor the instance belongs to, indexing Outline.Styles; MODEL_INVALID for an instance with no actor.
	uint actor_;

	uint model_shading_padding_0_;
};

/**
* [EN]
* skin_vertex_buffer_index_ is the bindless SRV of the CompressedModelSkin
* buffer (0xFFFFFFFF for a static, unskinned instance).
*/
struct ModelSkining                       // 1 row
{
	uint skin_index_;
	uint bone_offset_;
	uint skin_vertex_buffer_index_;
	uint model_skining_padding_0_;
};

/**
* [EN]
* Raster morph blend (see the model mesh shaders and
* Model/Material/MaterialResolveCS.hlsl/Model/Transparent/TransparentMS.hlsl).
* morph_delta_offset_ is in float3 units into morph_delta_buffer_index_; morph_vertex_offset_/
* morph_vertex_count_ are this SubMesh's Crister::vertices_ range start and
* ORIGINAL (pre-LOD) vertex count; morph_weight_offset_ indexes into the
* shared per-frame morph weight buffer. morph_target_count_ == 0 disables
* blending entirely - always the case for a non-morphed instance, and also
* for a morphed instance drawn from an own-page (streamed-in, non-pool)
* cluster: only the LOD 0 shared vertex pool range is index-aligned with
* morph_delta_buffer_index_/vertex_morph_source_buffer_index_'s
* crister-wide numbering (see Crister::vertexMorphSource_'s comment), so
* ModelRenderer leaves these zeroed for any other cluster and that
* instance's coarser LOD renders its frozen bind-pose shape instead of
* morphing.
*/
struct ModelMorphing                         // 2 rows
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

/**
* [EN]
* Base PBR values and their matching bindless texture SRVs. ior_ is
* KHR_materials_ior; emissive_strength_ is KHR_materials_emissive_strength.
* All four base textures plus occlusion_texture_index_ are sampled by
* Model/Opaque/DeferredLightingPS.hlsl using the model UV recovered from
* the VisibilityBuffer (RT4.zw) - the deferred pass has no interpolated
* texcoord of its own.
*/
struct ModelTexture                       // 5 rows
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

/**
* [EN]
* KHR extension material values/textures (specular, clearcoat, anisotropy,
* transmission, volume, sheen, iridescence, unlit). Each texture multiplies
* its matching factor per the glTF spec's channel convention; 0xFFFFFFFF
* means absent.
*/
struct ModelExtension                     // 9 rows
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

/**
* [EN]
* Per-instance StructuredBuffer element read by every Model pass (depth
* prepass, shadow, G-buffer, deferred lighting, GI/reflection/refraction
* raytracing, OIT). Grouped by concern so the cheap, frequently-run passes
* (depth/shadow/culling) only ever touch transform_/geometry_/streaming_ -
* those three sit contiguously at the front so they share the fewest cache
* lines, while the material-heavy texture_/extension_ groups (used only by
* the lighting passes) sit at the back.
*/
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

/**
* [EN]
* Fully expanded vertex, produced by DecodeModelVertex from a
* CompressedModelVertex. Skinning attributes live in the separate
* CompressedModelSkin buffer (skinned models only), not here.
*/
struct ExpandedModelVertex
{
	float3 position_;
	float3 normal_;
	float4 tangent_;
	float2 texcoord_;
};

/**
* [EN]
* Mirrors the C++ CompressedVertex (Model/Crister/CristerGeometry.h) - 16
* bytes. position_xy_/position_z_texu_ pack position (UNORM in the instance's
* position AABB) and texcoord u (UNORM in the instance's UV AABB) as 16-bit
* fields; texv_tangent_ packs texcoord v plus an octahedral-encoded tangent
* (x:8 y:7) and its handedness sign (bit 15); normal_ is the
* octahedral-encoded normal (x:16 y:16).
*/
struct CompressedModelVertex
{
	uint position_xy_;
	uint position_z_texu_;
	uint texv_tangent_;
	uint normal_;
};

/**
* [EN]
* Mirrors the C++ CompressedSkinVertex (Model/Crister/CristerGeometry.h) - 12
* bytes. joints_xy_/joints_zw_ pack the four influencing joint indices as
* 16-bit fields; weights_ packs their four blend weights as 8-bit UNORM.
*/
struct CompressedModelSkin
{
	uint joints_xy_;
	uint joints_zw_;
	uint weights_;
};

/**
* [EN]
* Unpacked skinning attributes, produced by DecodeSkinVertex from a
* CompressedModelSkin. weights_ is renormalised because four 8-bit UNORM
* values rarely sum to exactly one.
*/
struct ExpandedModelSkin
{
	uint4 joints_;
	float4 weights_;
};

/**
* [EN]
* One meshlet's range into the shared vertex-index/primitive-index buffers
* (ModelGeometry::vertex_indices_buffer_index_/primitive_indices_buffer_index_).
*/
struct ModelMeshlet
{
	uint vertex_offset_;
	uint triangle_offset_;
	uint vertex_count_;
	uint triangle_count_;
};

/**
* [EN]
* Bounding sphere (center_/radius_) plus normal cone (cone_axis_/
* cone_cutoff_) of one meshlet in object space, read by the meshlet culling.
*/
struct ModelMeshletBound
{
	float3 center_;
	float radius_;
	float3 cone_axis_;
	float cone_cutoff_;
};

/**
* [EN]
* Bone palette element (inverse bind matrix x joint global transform), stored
* as the engine Matrix (row-major, row-vector convention). Kept as four
* explicit row vectors so the layout does not depend on matrix-orientation
* annotations inside structured buffers - see LoadBoneMatrix.
*/
struct ModelBoneMatrix
{
	float4 row0_;
	float4 row1_;
	float4 row2_;
	float4 row3_;
};

/**
* [EN]
* Dequantises a CompressedModelVertex with the instance's AABBs
* (instance.streaming_). Matches the CPU encode in Crister.cpp exactly.
*/
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

/**
* [EN]
* Unpacks a CompressedModelSkin's joints/weights.
*/
ExpandedModelSkin DecodeSkinVertex(CompressedModelSkin packed)
{
	ExpandedModelSkin skin;
	skin.joints_ = uint4(packed.joints_xy_ & 0xFFFF, packed.joints_xy_ >> 16, packed.joints_zw_ & 0xFFFF, packed.joints_zw_ >> 16);
	skin.weights_ = float4(packed.weights_ & 0xFF, (packed.weights_ >> 8) & 0xFF, (packed.weights_ >> 16) & 0xFF, packed.weights_ >> 24) / 255.0;
	skin.weights_ /= max(dot(skin.weights_, 1.0), 1e-6);
	return skin;
}

/**
* [EN]
* Rebuilds the row-vector-convention matrix (use as mul(vector, matrix)).
* The float4x4 vector constructor takes rows, so this is orientation-safe.
*/
float4x4 LoadBoneMatrix(ModelBoneMatrix bone)
{
	return float4x4(bone.row0_, bone.row1_, bone.row2_, bone.row3_);
}

/**
* [EN]
* Packs a triangle's Visibility-Buffer identity: x is the index into
* ModelInstances plus one, so the cleared value 0 means no triangle covers
* the pixel; y keeps the low 7 bits for triangle_in_meshlet_index
* (meshlets emit at most 124 triangles) and the high bits for meshlet_index.
* The texcoord is written separately to VisibilityTexcoord.
*/
uint2 PackVisibilityID(uint instance_index, uint meshlet_index, uint triangle_in_meshlet_index)
{
	return uint2(instance_index + 1, (meshlet_index << 7) | (triangle_in_meshlet_index & 0x7F));
}

/**
* [EN]
* Unpacks the instance/meshlet/triangle-in-meshlet fields packed by
* PackVisibilityID. Callers check packed.x for 0 (no triangle) first.
*/
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

#endif // __MODEL_HLSL__
