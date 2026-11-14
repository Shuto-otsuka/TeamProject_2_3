#include <GraphicsEngine/Model/Opaque/Geometry.hlsli>
#include <GraphicsEngine/Model/Material/MaterialMapping.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

/**
* [EN]
* Depth passes (Geometry.DepthEarly / Geometry.DepthLate): writes nothing but
* depth, and discards the pixels a masked material cuts out, so holes never
* occlude what lies behind them. The base color alpha is sampled with the
* material's mapping; non-stretch mappings take their inputs from the
* deformed surface the geometry stage interpolates, facing the camera (the
* face normal comes from the screen derivatives of the world position, taken
* before any per-instance branch). A fur shell layer also keeps only the
* strands that reach it: each texcoord cell hashes to a strand height, and a
* layer above that height is cut out.
*/
void Depth(GeometryOutput input, GeometryPrimitive primitive)
{
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[input.instance_index];
	float3 face_normal = cross(ddx(input.world_position), ddy(input.world_position));

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
			float3 object_normal = normalize(input.object_normal);
			float3 object_tangent = normalize(input.object_tangent.xyz);
			float3 world_normal = normalize(mul(float4(object_normal, 0.0), instance.transform_.inverse_transpose_world_).xyz);
			if (dot(face_normal, world_normal) < 0.0)
			{
				face_normal = -face_normal;
			}
			if (dot(face_normal, scene.camera_position_.xyz - input.world_position) <= 0.0)
			{
				object_normal = -object_normal;
				world_normal = -world_normal;
			}
			float3 object_bitangent = cross(object_normal, object_tangent) * input.object_tangent.w;
			float3 object_scale = float3(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz), length(instance.transform_.world_[2].xyz));
			mapping = ResolveMaterialMapping(instance.shading_.material_mapping_, instance.shading_.tiling_, instance.shading_.triplanar_sharpness_, input.texcoord, input.object_position, object_normal, object_tangent, object_bitangent, object_scale, input.world_position, world_normal);
		}
		Texture2D base_color_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.base_color_texture_index_)];
		alpha *= SampleMaterialMappingGrad(base_color_texture, sampler_linear_wrap, mapping).a;
	}
	clip(alpha - instance.texture_.alpha_cutoff_);

	if (instance.geometry_.layer_ != 0)
	{
		float fur_ratio = (float)instance.geometry_.layer_ / (float)instance.shading_.fur_shell_count_;
		float2 strand_cell = frac(floor(input.texcoord * (128.0 + instance.shading_.fur_density_ * 384.0)) * float2(123.34, 456.21));
		strand_cell += dot(strand_cell, strand_cell + 45.32);
		float strand_height = frac(strand_cell.x * strand_cell.y);
		clip(strand_height * strand_height - fur_ratio);
	}
}

/**
* [EN]
* Geometry.Visibility: re-rasterizes the early and late draws against the
* finished depth (equal-or-nearer test, no depth write) and stores which
* triangle covers each pixel plus its texcoord, after the same alpha and fur
* strand cutouts as the depth passes.
*/
void Visibility(GeometryOutput input, GeometryPrimitive primitive, out uint2 visibility_id : SV_Target0, out float2 visibility_uv : SV_Target1)
{
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[input.instance_index];
	float3 face_normal = cross(ddx(input.world_position), ddy(input.world_position));

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
			float3 object_normal = normalize(input.object_normal);
			float3 object_tangent = normalize(input.object_tangent.xyz);
			float3 world_normal = normalize(mul(float4(object_normal, 0.0), instance.transform_.inverse_transpose_world_).xyz);
			if (dot(face_normal, world_normal) < 0.0)
			{
				face_normal = -face_normal;
			}
			if (dot(face_normal, scene.camera_position_.xyz - input.world_position) <= 0.0)
			{
				object_normal = -object_normal;
				world_normal = -world_normal;
			}
			float3 object_bitangent = cross(object_normal, object_tangent) * input.object_tangent.w;
			float3 object_scale = float3(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz), length(instance.transform_.world_[2].xyz));
			mapping = ResolveMaterialMapping(instance.shading_.material_mapping_, instance.shading_.tiling_, instance.shading_.triplanar_sharpness_, input.texcoord, input.object_position, object_normal, object_tangent, object_bitangent, object_scale, input.world_position, world_normal);
		}
		Texture2D base_color_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.base_color_texture_index_)];
		alpha *= SampleMaterialMappingGrad(base_color_texture, sampler_linear_wrap, mapping).a;
	}
	clip(alpha - instance.texture_.alpha_cutoff_);

	if (instance.geometry_.layer_ != 0)
	{
		float fur_ratio = (float)instance.geometry_.layer_ / (float)instance.shading_.fur_shell_count_;
		float2 strand_cell = frac(floor(input.texcoord * (128.0 + instance.shading_.fur_density_ * 384.0)) * float2(123.34, 456.21));
		strand_cell += dot(strand_cell, strand_cell + 45.32);
		float strand_height = frac(strand_cell.x * strand_cell.y);
		clip(strand_height * strand_height - fur_ratio);
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	if (scene.view_mode_ == VIEW_MODE_MESHLET_CULLING)
	{
		RWTexture2D<uint> debug = ResourceDescriptorHeap[bindings.geometry_debug_];
		debug[uint2(input.position.xy)] = (dispatch_bindings.constants_ & CULLING_COMMAND_MASK) != CULLING_EARLY_ARGUMENTS ? DEBUG_GEOMETRY_LATE : 0;
	}

	visibility_id = PackVisibilityID(input.instance_index, input.meshlet_index, primitive.triangle_in_meshlet_index);
	visibility_uv = input.texcoord;
}
