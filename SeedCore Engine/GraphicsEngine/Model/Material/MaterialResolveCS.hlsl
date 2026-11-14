#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Model/Material/MaterialMapping.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

#define MATERIAL_SORT_BUCKET_COUNT 1024

struct MaterialResolveBindings
{
	uint material_pixels_;
	uint material_buckets_;
	uint geometry_visibility_id_;
	uint geometry_instances_;
	uint model_bones_;
	uint model_morphs_;
	uint material_base_color_;
	uint material_normal_roughness_;
	uint material_tangent_;
	uint material_velocity_;
	uint material_radiance_;
	uint material_mip_;
};
ConstantBuffer<MaterialResolveBindings> bindings : register(b0, space1);

/**
* [EN]
* Final step of the material sort: one thread per entry of
* Material.Pixels (rows of 65536 threads; the total sits after the
* buckets). Rebuilds the covering triangle from the same buffers the geometry
* passes read - morph targets, then skinning, for this frame and, with the
* second half of ModelBones / ModelMorphWeights, for the previous one, then
* the same fur shell push and droop as the geometry passes - and derives
* perspective-correct barycentric weights for the pixel and for its right
* and lower neighbours. The neighbours give the mapping coordinates'
* screen derivatives, so every texture is sampled with SampleGrad at the
* right mip. Writes the base color and metallic, the octahedral normal (12+12
* bits) with roughness (8 bits), the tangent angle, the velocity, and the
* emitted radiance that Material.Radiance starts from (emissive times its
* strength; base color plus emissive for Unlit). Normals face the camera,
* and the Flat shading model uses the geometric face normal.
*
* ---------------------------------------------------------------------
*
* [JP]
* マテリアルソートの最後。Material.Pixels の 1 件につき 1 スレッド
* (1 行 65536 スレッド。総数はバケットの後ろにある)。ジオメトリのパスと
* 同じバッファから三角形を組み直し（モーフ、次にスキン。ModelBones /
* ModelMorphWeights の後ろ半分で前のフレームの分も。そのあとジオメトリのパスと
* 同じ Fur のシェルの押し出しと垂れ）、ピクセルと右隣・下隣の
* パースペクティブ補正した重心座標を求める。隣の分がマッピングの座標の画面方向の
* 微分になり、テクスチャはすべて正しいミップの SampleGrad で読む。ベースカラーと
* メタリック、八面体法線（12+12 bit）とラフネス（8 bit）、タンジェントの角度、
* 速度、Material.Radiance の初期値の放射（エミッシブ×強さ。Unlit はベースカラー＋
* エミッシブ）を書く。法線はカメラの側へ向け、Flat のシェーディングモデルは
* 三角形の面の法線を使う。
*/
[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	StructuredBuffer<uint> buckets = ResourceDescriptorHeap[bindings.material_buckets_];
	uint entry_index = id.y * 65536 + id.x;
	if (entry_index >= buckets[MATERIAL_SORT_BUCKET_COUNT])
	{
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	Texture2D<uint2> visibility_id = ResourceDescriptorHeap[bindings.geometry_visibility_id_];
	uint width;
	uint height;
	visibility_id.GetDimensions(width, height);

	StructuredBuffer<uint> pixels = ResourceDescriptorHeap[bindings.material_pixels_];
	uint linear_pixel = pixels[entry_index];
	uint2 pixel = uint2(linear_pixel % width, linear_pixel / width);

	uint instance_index;
	uint meshlet_index;
	uint triangle_in_meshlet_index;
	UnpackVisibilityID(visibility_id.Load(int3(pixel, 0)), instance_index, meshlet_index, triangle_in_meshlet_index);

	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[instance_index];

	if (instance.geometry_.meshlet_count_ == 0)
	{
		RWTexture2D<float4> empty_base_color = ResourceDescriptorHeap[bindings.material_base_color_];
		RWTexture2D<uint> empty_normal_roughness = ResourceDescriptorHeap[bindings.material_normal_roughness_];
		RWTexture2D<uint> empty_tangent = ResourceDescriptorHeap[bindings.material_tangent_];
		RWTexture2D<float2> empty_velocity = ResourceDescriptorHeap[bindings.material_velocity_];
		RWTexture2D<float4> empty_radiance = ResourceDescriptorHeap[bindings.material_radiance_];
		empty_base_color[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		empty_normal_roughness[pixel] = 0;
		empty_tangent[pixel] = 0;
		empty_velocity[pixel] = float2(0.0, 0.0);
		empty_radiance[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	StructuredBuffer<CompressedModelVertex> vertices = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.vertex_buffer_index_)];
	StructuredBuffer<ModelMeshlet> meshlets = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.meshlet_buffer_index_)];
	StructuredBuffer<uint> vertex_indices = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.vertex_indices_buffer_index_)];
	ByteAddressBuffer primitive_indices = ResourceDescriptorHeap[NonUniformResourceIndex(instance.geometry_.primitive_indices_buffer_index_)];
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
	float3 previous_local_position0 = local_position0;
	float3 previous_local_position1 = local_position1;
	float3 previous_local_position2 = local_position2;

	if (instance.morph_.morph_target_count_ != 0)
	{
		StructuredBuffer<uint> vertex_morph_source = ResourceDescriptorHeap[NonUniformResourceIndex(instance.morph_.vertex_morph_source_buffer_index_)];
		StructuredBuffer<float3> morph_deltas = ResourceDescriptorHeap[NonUniformResourceIndex(instance.morph_.morph_delta_buffer_index_)];
		StructuredBuffer<float> morphs = ResourceDescriptorHeap[bindings.model_morphs_];
		uint weight_count;
		uint weight_stride;
		morphs.GetDimensions(weight_count, weight_stride);
		uint previous_weight_base = weight_count / 2;

		uint morph_vertex_index0 = vertex_morph_source[global_index0] - instance.morph_.morph_vertex_offset_;
		uint morph_vertex_index1 = vertex_morph_source[global_index1] - instance.morph_.morph_vertex_offset_;
		uint morph_vertex_index2 = vertex_morph_source[global_index2] - instance.morph_.morph_vertex_offset_;
		for (uint target = 0; target < instance.morph_.morph_target_count_; target++)
		{
			float weight = morphs[instance.morph_.morph_weight_offset_ + target];
			float previous_weight = morphs[previous_weight_base + instance.morph_.morph_weight_offset_ + target];
			float3 morph_delta0 = morph_deltas[instance.morph_.morph_delta_offset_ + target * instance.morph_.morph_vertex_count_ + morph_vertex_index0];
			float3 morph_delta1 = morph_deltas[instance.morph_.morph_delta_offset_ + target * instance.morph_.morph_vertex_count_ + morph_vertex_index1];
			float3 morph_delta2 = morph_deltas[instance.morph_.morph_delta_offset_ + target * instance.morph_.morph_vertex_count_ + morph_vertex_index2];
			local_position0 += morph_delta0 * weight;
			local_position1 += morph_delta1 * weight;
			local_position2 += morph_delta2 * weight;
			previous_local_position0 += morph_delta0 * previous_weight;
			previous_local_position1 += morph_delta1 * previous_weight;
			previous_local_position2 += morph_delta2 * previous_weight;
		}
	}

	float3 local_normal0 = vertex0.normal_;
	float3 local_normal1 = vertex1.normal_;
	float3 local_normal2 = vertex2.normal_;
	float3 local_tangent0 = vertex0.tangent_.xyz;
	float3 local_tangent1 = vertex1.tangent_.xyz;
	float3 local_tangent2 = vertex2.tangent_.xyz;

	if (instance.skining_.skin_index_ != MODEL_INVALID)
	{
		StructuredBuffer<ModelBoneMatrix> bone_matrices = ResourceDescriptorHeap[bindings.model_bones_];
		StructuredBuffer<CompressedModelSkin> skin_vertices = ResourceDescriptorHeap[NonUniformResourceIndex(instance.skining_.skin_vertex_buffer_index_)];
		uint bone_count;
		uint bone_stride;
		bone_matrices.GetDimensions(bone_count, bone_stride);
		uint bone_base = instance.skining_.bone_offset_;
		uint previous_bone_base = bone_count / 2 + instance.skining_.bone_offset_;

		ExpandedModelSkin skin0 = DecodeSkinVertex(skin_vertices[global_index0]);
		ExpandedModelSkin skin1 = DecodeSkinVertex(skin_vertices[global_index1]);
		ExpandedModelSkin skin2 = DecodeSkinVertex(skin_vertices[global_index2]);

		float4x4 skin_matrix0 =
			LoadBoneMatrix(bone_matrices[bone_base + skin0.joints_.x]) * skin0.weights_.x +
			LoadBoneMatrix(bone_matrices[bone_base + skin0.joints_.y]) * skin0.weights_.y +
			LoadBoneMatrix(bone_matrices[bone_base + skin0.joints_.z]) * skin0.weights_.z +
			LoadBoneMatrix(bone_matrices[bone_base + skin0.joints_.w]) * skin0.weights_.w;
		float4x4 skin_matrix1 =
			LoadBoneMatrix(bone_matrices[bone_base + skin1.joints_.x]) * skin1.weights_.x +
			LoadBoneMatrix(bone_matrices[bone_base + skin1.joints_.y]) * skin1.weights_.y +
			LoadBoneMatrix(bone_matrices[bone_base + skin1.joints_.z]) * skin1.weights_.z +
			LoadBoneMatrix(bone_matrices[bone_base + skin1.joints_.w]) * skin1.weights_.w;
		float4x4 skin_matrix2 =
			LoadBoneMatrix(bone_matrices[bone_base + skin2.joints_.x]) * skin2.weights_.x +
			LoadBoneMatrix(bone_matrices[bone_base + skin2.joints_.y]) * skin2.weights_.y +
			LoadBoneMatrix(bone_matrices[bone_base + skin2.joints_.z]) * skin2.weights_.z +
			LoadBoneMatrix(bone_matrices[bone_base + skin2.joints_.w]) * skin2.weights_.w;
		float4x4 previous_skin_matrix0 =
			LoadBoneMatrix(bone_matrices[previous_bone_base + skin0.joints_.x]) * skin0.weights_.x +
			LoadBoneMatrix(bone_matrices[previous_bone_base + skin0.joints_.y]) * skin0.weights_.y +
			LoadBoneMatrix(bone_matrices[previous_bone_base + skin0.joints_.z]) * skin0.weights_.z +
			LoadBoneMatrix(bone_matrices[previous_bone_base + skin0.joints_.w]) * skin0.weights_.w;
		float4x4 previous_skin_matrix1 =
			LoadBoneMatrix(bone_matrices[previous_bone_base + skin1.joints_.x]) * skin1.weights_.x +
			LoadBoneMatrix(bone_matrices[previous_bone_base + skin1.joints_.y]) * skin1.weights_.y +
			LoadBoneMatrix(bone_matrices[previous_bone_base + skin1.joints_.z]) * skin1.weights_.z +
			LoadBoneMatrix(bone_matrices[previous_bone_base + skin1.joints_.w]) * skin1.weights_.w;
		float4x4 previous_skin_matrix2 =
			LoadBoneMatrix(bone_matrices[previous_bone_base + skin2.joints_.x]) * skin2.weights_.x +
			LoadBoneMatrix(bone_matrices[previous_bone_base + skin2.joints_.y]) * skin2.weights_.y +
			LoadBoneMatrix(bone_matrices[previous_bone_base + skin2.joints_.z]) * skin2.weights_.z +
			LoadBoneMatrix(bone_matrices[previous_bone_base + skin2.joints_.w]) * skin2.weights_.w;

		local_position0 = mul(float4(local_position0, 1.0), skin_matrix0).xyz;
		local_position1 = mul(float4(local_position1, 1.0), skin_matrix1).xyz;
		local_position2 = mul(float4(local_position2, 1.0), skin_matrix2).xyz;
		previous_local_position0 = mul(float4(previous_local_position0, 1.0), previous_skin_matrix0).xyz;
		previous_local_position1 = mul(float4(previous_local_position1, 1.0), previous_skin_matrix1).xyz;
		previous_local_position2 = mul(float4(previous_local_position2, 1.0), previous_skin_matrix2).xyz;
		local_normal0 = normalize(mul(float4(local_normal0, 0.0), skin_matrix0).xyz);
		local_normal1 = normalize(mul(float4(local_normal1, 0.0), skin_matrix1).xyz);
		local_normal2 = normalize(mul(float4(local_normal2, 0.0), skin_matrix2).xyz);
		local_tangent0 = normalize(mul(float4(local_tangent0, 0.0), skin_matrix0).xyz);
		local_tangent1 = normalize(mul(float4(local_tangent1, 0.0), skin_matrix1).xyz);
		local_tangent2 = normalize(mul(float4(local_tangent2, 0.0), skin_matrix2).xyz);
	}

	float fur_ratio = 0.0;
	if (instance.geometry_.layer_ != 0)
	{
		fur_ratio = (float)instance.geometry_.layer_ / (float)instance.shading_.fur_shell_count_;
		float fur_height = pow(fur_ratio, 1.35) * instance.shading_.fur_length_;
		local_position0 += normalize(local_normal0) * fur_height;
		local_position1 += normalize(local_normal1) * fur_height;
		local_position2 += normalize(local_normal2) * fur_height;
		previous_local_position0 += normalize(local_normal0) * fur_height;
		previous_local_position1 += normalize(local_normal1) * fur_height;
		previous_local_position2 += normalize(local_normal2) * fur_height;
	}
	float3 fur_droop = float3(0.0, fur_ratio * fur_ratio * instance.shading_.fur_length_ * 0.35, 0.0);

	float3 world_position0 = mul(float4(local_position0, 1.0), instance.transform_.world_).xyz - fur_droop;
	float3 world_position1 = mul(float4(local_position1, 1.0), instance.transform_.world_).xyz - fur_droop;
	float3 world_position2 = mul(float4(local_position2, 1.0), instance.transform_.world_).xyz - fur_droop;
	float4 clip0 = mul(float4(world_position0, 1.0), scene.current_view_projection_);
	float4 clip1 = mul(float4(world_position1, 1.0), scene.current_view_projection_);
	float4 clip2 = mul(float4(world_position2, 1.0), scene.current_view_projection_);
	float2 ndc0 = clip0.xy / clip0.w;
	float2 ndc1 = clip1.xy / clip1.w;
	float2 ndc2 = clip2.xy / clip2.w;
	float area = (ndc1.x - ndc0.x) * (ndc2.y - ndc0.y) - (ndc1.y - ndc0.y) * (ndc2.x - ndc0.x);

	float2 pixel_ndc = (float2(pixel) + 0.5) / float2(width, height);
	pixel_ndc = float2(pixel_ndc.x * 2.0 - 1.0, 1.0 - pixel_ndc.y * 2.0);
	float2 sample_ndc[3] = { pixel_ndc, pixel_ndc + float2(2.0 / width, 0.0), pixel_ndc - float2(0.0, 2.0 / height) };
	float3 weights[3];
	for (uint sample_index = 0; sample_index < 3; sample_index++)
	{
		float2 ndc = sample_ndc[sample_index];
		float edge0 = (ndc2.x - ndc1.x) * (ndc.y - ndc1.y) - (ndc2.y - ndc1.y) * (ndc.x - ndc1.x);
		float edge1 = (ndc0.x - ndc2.x) * (ndc.y - ndc2.y) - (ndc0.y - ndc2.y) * (ndc.x - ndc2.x);
		float edge2 = (ndc1.x - ndc0.x) * (ndc.y - ndc0.y) - (ndc1.y - ndc0.y) * (ndc.x - ndc0.x);
		float3 perspective = float3(edge0 / clip0.w, edge1 / clip1.w, edge2 / clip2.w) / area;
		weights[sample_index] = perspective / (perspective.x + perspective.y + perspective.z);
	}
	float3 weight = weights[0];

	float3 world_position = weight.x * world_position0 + weight.y * world_position1 + weight.z * world_position2;
	float3 previous_world_position0 = mul(float4(previous_local_position0, 1.0), instance.transform_.previous_world_).xyz - fur_droop;
	float3 previous_world_position1 = mul(float4(previous_local_position1, 1.0), instance.transform_.previous_world_).xyz - fur_droop;
	float3 previous_world_position2 = mul(float4(previous_local_position2, 1.0), instance.transform_.previous_world_).xyz - fur_droop;
	float3 previous_world_position = weight.x * previous_world_position0 + weight.y * previous_world_position1 + weight.z * previous_world_position2;

	float3 object_normal = normalize(weight.x * local_normal0 + weight.y * local_normal1 + weight.z * local_normal2);
	float3 object_tangent = normalize(weight.x * local_tangent0 + weight.y * local_tangent1 + weight.z * local_tangent2);
	float tangent_sign = vertex0.tangent_.w;
	float3 normal = normalize(mul(float4(object_normal, 0.0), instance.transform_.inverse_transpose_world_).xyz);
	float3 tangent = normalize(mul(float4(object_tangent, 0.0), instance.transform_.world_).xyz);

	float3 face_normal = cross(world_position1 - world_position0, world_position2 - world_position0);
	if (dot(face_normal, normal) < 0.0)
	{
		face_normal = -face_normal;
	}
	if (instance.shading_.shading_model_ == SHADING_MODEL_FLAT)
	{
		normal = normalize(face_normal);
	}
	if (dot(face_normal, scene.camera_position_.xyz - world_position) <= 0.0)
	{
		normal = -normal;
		object_normal = -object_normal;
	}

	float3 object_bitangent = cross(object_normal, object_tangent) * tangent_sign;
	float3 object_scale = float3(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz), length(instance.transform_.world_[2].xyz));
	MaterialMappingCoordinate mappings[3];
	for (uint mapping_index = 0; mapping_index < 3; mapping_index++)
	{
		float3 sample_weight = weights[mapping_index];
		float2 sample_texcoord = sample_weight.x * vertex0.texcoord_ + sample_weight.y * vertex1.texcoord_ + sample_weight.z * vertex2.texcoord_;
		float3 sample_object_position = sample_weight.x * local_position0 + sample_weight.y * local_position1 + sample_weight.z * local_position2;
		float3 sample_world_position = sample_weight.x * world_position0 + sample_weight.y * world_position1 + sample_weight.z * world_position2;
		mappings[mapping_index] = ResolveMaterialMapping(instance.shading_.material_mapping_, instance.shading_.tiling_, instance.shading_.triplanar_sharpness_, sample_texcoord, sample_object_position, object_normal, object_tangent, object_bitangent, object_scale, sample_world_position, normal);
	}

	float4 base_color = instance.texture_.base_color_;
	if (instance.texture_.base_color_texture_index_ != MODEL_INVALID)
	{
		Texture2D base_color_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.base_color_texture_index_)];
		base_color *= SampleMaterialMappingGrad(base_color_texture, sampler_linear_wrap, mappings[0], mappings[1], mappings[2]);
		if (scene.view_mode_ == VIEW_MODE_TEXTURE_MIP)
		{
			uint texture_width;
			uint texture_height;
			uint texture_levels;
			base_color_texture.GetDimensions(0, texture_width, texture_height, texture_levels);
			float2 texel_dx = (mappings[1].texcoord_x_ - mappings[0].texcoord_x_) * float2(texture_width, texture_height);
			float2 texel_dy = (mappings[2].texcoord_x_ - mappings[0].texcoord_x_) * float2(texture_width, texture_height);
			RWTexture2D<float> mip = ResourceDescriptorHeap[bindings.material_mip_];
			mip[pixel] = 0.5 * log2(max(max(dot(texel_dx, texel_dx), dot(texel_dy, texel_dy)), 1e-8)) + 1.0;
		}
	}
	else if (scene.view_mode_ == VIEW_MODE_TEXTURE_MIP)
	{
		RWTexture2D<float> mip = ResourceDescriptorHeap[bindings.material_mip_];
		mip[pixel] = 0.0;
	}

	float3 shading_normal = normal;
	if (instance.texture_.normal_texture_index_ != MODEL_INVALID)
	{
		Texture2D normal_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.normal_texture_index_)];
		if (instance.shading_.material_mapping_ == MATERIAL_MAPPING_TRIPLANAR_OBJECT)
		{
			float3 projection_normal = SampleNormalMappingGrad(normal_texture, sampler_linear_wrap, mappings[0], mappings[1], mappings[2]);
			shading_normal = normalize(mul(float4(projection_normal * object_scale, 0.0), instance.transform_.inverse_transpose_world_).xyz);
		}
		else if (instance.shading_.material_mapping_ == MATERIAL_MAPPING_TRIPLANAR_WORLD)
		{
			shading_normal = SampleNormalMappingGrad(normal_texture, sampler_linear_wrap, mappings[0], mappings[1], mappings[2]);
		}
		else
		{
			float3 bitangent = cross(normal, tangent) * tangent_sign;
			float3 tangent_normal = SampleMaterialMappingGrad(normal_texture, sampler_linear_wrap, mappings[0], mappings[1], mappings[2]).xyz * 2.0 - 1.0;
			shading_normal = normalize(mul(tangent_normal, float3x3(tangent, bitangent, normal)));
		}
	}

	float metallic = instance.texture_.metallic_;
	float roughness = instance.texture_.roughness_;
	if (instance.texture_.metallic_roughness_texture_index_ != MODEL_INVALID)
	{
		Texture2D metallic_roughness_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.metallic_roughness_texture_index_)];
		float4 metallic_roughness = SampleMaterialMappingGrad(metallic_roughness_texture, sampler_linear_wrap, mappings[0], mappings[1], mappings[2]);
		metallic *= metallic_roughness.b;
		roughness *= metallic_roughness.g;
	}

	float3 emissive = instance.texture_.emissive_;
	if (instance.texture_.emissive_texture_index_ != MODEL_INVALID)
	{
		Texture2D emissive_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.emissive_texture_index_)];
		emissive *= SampleMaterialMappingGrad(emissive_texture, sampler_linear_wrap, mappings[0], mappings[1], mappings[2]).rgb;
	}
	emissive *= instance.texture_.emissive_strength_;

	float4 current_clip = mul(float4(world_position, 1.0), scene.current_view_projection_);
	float4 previous_clip = mul(float4(previous_world_position, 1.0), scene.previous_view_projection_);

	uint2 octahedral = uint2(saturate(OctNormalEncode(shading_normal)) * 4095.0 + 0.5);
	uint roughness_bits = uint(saturate(roughness) * 255.0 + 0.5);

	RWTexture2D<float4> surface_base_color = ResourceDescriptorHeap[bindings.material_base_color_];
	RWTexture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_normal_roughness_];
	RWTexture2D<uint> surface_tangent = ResourceDescriptorHeap[bindings.material_tangent_];
	RWTexture2D<float2> velocity = ResourceDescriptorHeap[bindings.material_velocity_];
	RWTexture2D<float4> surface_radiance = ResourceDescriptorHeap[bindings.material_radiance_];
	surface_base_color[pixel] = float4(base_color.rgb, metallic);
	surface_normal_roughness[pixel] = octahedral.x | (octahedral.y << 12) | (roughness_bits << 24);
	surface_tangent[pixel] = uint(PackTangentAngle(shading_normal, tangent, tangent_sign) * 65535.0 + 0.5);
	velocity[pixel] = (current_clip.xy / current_clip.w - previous_clip.xy / previous_clip.w) * 0.5;
	surface_radiance[pixel] = float4(instance.shading_.shading_model_ == SHADING_MODEL_UNLIT ? base_color.rgb + emissive : emissive, 0.0);
}
