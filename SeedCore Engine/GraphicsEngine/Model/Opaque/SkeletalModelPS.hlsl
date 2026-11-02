#include "../Model.hlsli"
#include "../../Shader/ShaderResources.hlsli"
#include "../../Shader/Sampler.hlsli"
#include "../../Shader/Scene.hlsli"
#include "../Material/MaterialMapping.hlsli"

/**
* [EN]
* Pixel Shader for skeletal model G-Buffer output - VisibilityBuffer id only.
* Identical to StaticModelPS.
*
* ---------------------------------------------------------------------
*
* [JP]
* スケルタルモデル G-Buffer 出力用のピクセルシェーダー - VisibilityBuffer id
* のみ。StaticModelPS と同一。
*/
ModelPSOutput main(ModelMSOutput input, ModelMSPrimitiveOutput primitive)
{
	StructuredBuffer<ModelStructuredBuffer> instances = GetModelStructuredBuffer(shader_resource_indices.model_.instance_index_);
	ModelStructuredBuffer instance = instances[input.instance_index];

	float4 base_color = instance.texture_.base_color_;
	if (instance.texture_.base_color_texture_index_ != 0xFFFFFFFF && instance.texture_.alpha_cutoff_ > 0.0)
	{
		MaterialMappingCoordinate mapping;
		mapping.texcoord_x_ = input.texcoord;
		mapping.texcoord_y_ = input.texcoord;
		mapping.texcoord_z_ = input.texcoord;
		mapping.weight_ = float3(1.0, 0.0, 0.0);
		mapping.projection_normal_ = float3(0.0, 0.0, 1.0);
		if (instance.shading_.material_mapping_ != MATERIAL_MAPPING_STRETCH)
		{
			SceneConstantBuffer scene = GetSceneConstantBuffer();
			float2 pixel_uv = input.position.xy * scene.inverse_screen_size_;
			float2 pixel_ndc = float2(pixel_uv.x * 2.0 - 1.0, 1.0 - pixel_uv.y * 2.0);
			MaterialMappingSurface surface = ReconstructMaterialMapping(instance, input.meshlet_index, primitive.triangle_in_meshlet_index, pixel_ndc, scene.current_view_projection_, scene.camera_position_.xyz);
			float3 object_bitangent = cross(surface.object_normal_, surface.object_tangent_.xyz) * surface.object_tangent_.w;
			float3 object_scale = float3(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz), length(instance.transform_.world_[2].xyz));
			mapping = ResolveMaterialMapping(instance.shading_.material_mapping_, instance.shading_.tiling_, instance.shading_.triplanar_sharpness_, surface.texcoord_, surface.object_position_, surface.object_normal_, surface.object_tangent_.xyz, object_bitangent, object_scale, surface.world_position_, surface.world_normal_);
		}

		Texture2D base_color_texture = ResourceDescriptorHeap[instance.texture_.base_color_texture_index_];
		base_color *= SampleMaterialMappingGrad(base_color_texture, sampler_aniso_wrap, mapping);
	}

	clip(base_color.a - instance.texture_.alpha_cutoff_);

	ModelPSOutput output;
	output.visibility_id = PackVisibilityID(input.instance_index, input.meshlet_index, primitive.triangle_in_meshlet_index, input.texcoord);

	return output;
}
