#include <GraphicsEngine/Model/Opaque/Geometry.hlsli>
#include <GraphicsEngine/Model/Material/MaterialMapping.hlsli>

/**
* [EN]
* Depth passes (Geometry.DepthEarly / Geometry.DepthLate): writes nothing but
* depth, and discards the pixels a masked material cuts out, so holes never
* occlude what lies behind them. The base color alpha is sampled with the
* material's mapping; non-stretch mappings rebuild the surface from the
* triangle to get the mapping inputs.
*/
void Depth(GeometryOutput input, GeometryPrimitive primitive)
{
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.model_instances_];
	ModelStructuredBuffer instance = instances[input.instance_index];

	float alpha = instance.texture_.base_color_.a;
	if (instance.texture_.base_color_texture_index_ != MODEL_INVALID && instance.texture_.alpha_cutoff_ > 0.0)
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
			MaterialMappingSurface surface = ReconstructMaterialMapping(instance, input.meshlet_index, primitive.triangle_in_meshlet_index, pixel_ndc, scene.current_view_projection_, scene.camera_position_.xyz, bindings.model_bones_, bindings.model_morph_weights_);
			float3 object_bitangent = cross(surface.object_normal_, surface.object_tangent_.xyz) * surface.object_tangent_.w;
			float3 object_scale = float3(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz), length(instance.transform_.world_[2].xyz));
			mapping = ResolveMaterialMapping(instance.shading_.material_mapping_, instance.shading_.tiling_, instance.shading_.triplanar_sharpness_, surface.texcoord_, surface.object_position_, surface.object_normal_, surface.object_tangent_.xyz, object_bitangent, object_scale, surface.world_position_, surface.world_normal_);
		}
		Texture2D base_color_texture = ResourceDescriptorHeap[instance.texture_.base_color_texture_index_];
		alpha *= SampleMaterialMappingGrad(base_color_texture, sampler_linear_wrap, mapping).a;
	}
	clip(alpha - instance.texture_.alpha_cutoff_);
}

/**
* [EN]
* Geometry.Visibility: re-rasterizes the early and late draws against the
* finished depth (equal-or-nearer test, no depth write) and stores which
* triangle covers each pixel plus its texcoord, after the same alpha cutout as
* the depth passes.
*/
void Visibility(GeometryOutput input, GeometryPrimitive primitive, out uint2 visibility_id : SV_Target0, out float2 visibility_texcoord : SV_Target1)
{
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.model_instances_];
	ModelStructuredBuffer instance = instances[input.instance_index];

	float alpha = instance.texture_.base_color_.a;
	if (instance.texture_.base_color_texture_index_ != MODEL_INVALID && instance.texture_.alpha_cutoff_ > 0.0)
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
			MaterialMappingSurface surface = ReconstructMaterialMapping(instance, input.meshlet_index, primitive.triangle_in_meshlet_index, pixel_ndc, scene.current_view_projection_, scene.camera_position_.xyz, bindings.model_bones_, bindings.model_morph_weights_);
			float3 object_bitangent = cross(surface.object_normal_, surface.object_tangent_.xyz) * surface.object_tangent_.w;
			float3 object_scale = float3(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz), length(instance.transform_.world_[2].xyz));
			mapping = ResolveMaterialMapping(instance.shading_.material_mapping_, instance.shading_.tiling_, instance.shading_.triplanar_sharpness_, surface.texcoord_, surface.object_position_, surface.object_normal_, surface.object_tangent_.xyz, object_bitangent, object_scale, surface.world_position_, surface.world_normal_);
		}
		Texture2D base_color_texture = ResourceDescriptorHeap[instance.texture_.base_color_texture_index_];
		alpha *= SampleMaterialMappingGrad(base_color_texture, sampler_linear_wrap, mapping).a;
	}
	clip(alpha - instance.texture_.alpha_cutoff_);

	visibility_id = PackVisibilityID(input.instance_index, input.meshlet_index, primitive.triangle_in_meshlet_index);
	visibility_texcoord = input.texcoord;
}
