#ifndef __MATERIAL_HLSL__
#define __MATERIAL_HLSL__

#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Model/Material/MaterialMapping.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>
#include <GraphicsEngine/Shader/Vertex.hlsli>

// One entry per material slot in the mesh's Crister::Materials() list.
// Uploaded once per unique Crister (RaytracingRenderer::BuildReflectionMaterialTable,
// cached like the BLAS - not rebuilt every frame). Must match the C++ mirror in
// Renderer/RaytracingRenderer.h byte-for-byte.
struct ReflectionMaterial
{
	float3 base_color_;

	// Bindless SRV of that material's base-color texture, or 0xFFFFFFFF when it
	// has none / is not resident yet (CristerResidency::TextureBindlessIndex returns the
	// sentinel while a streaming mip is still loading).
	uint base_color_texture_index_;

	// KHR_materials_ior/transmission/volume - read by Refraction/RefractionRT.hlsl
	// (Snell refraction needs ior_; the Fresnel/transmit-vs-absorb decision needs
	// transmission_factor_; Beer-Lambert absorption over the traveled distance
	// inside the medium needs volume_attenuation_color_/distance_). Reflection
	// itself does not use these - they live here rather than in a separate
	// table so ResolveReflectionMaterial's per-triangle lookup can be shared.
	float ior_;
	float transmission_factor_;
	float3 volume_attenuation_color_;
	float volume_attenuation_distance_;

	// glTF alphaMode in the loader's encoding: 0 OPAQUE, 1 MASK, 2 BLEND.
	uint alpha_mode_;
	float alpha_cutoff_;
	float base_color_alpha_;

	// KHR_materials_volume thickness. Zero means THIN-WALLED: the surface
	// encloses no volume, so Refraction/RefractionRT.hlsl must skip Beer-Lambert
	// absorption entirely for it - otherwise a pane of window glass gets tinted
	// like a solid block of the same material.
	float thickness_factor_;
	uint thickness_texture_index_;   // .g scales thickness_factor_ per pixel

	uint material_mapping_;
	float tiling_;
	float triplanar_sharpness_;
	float material_padding_0_;
	float material_padding_1_;
};

// One entry per TLAS instance, indexed by InstanceID() in the closesthit
// shader. Uploaded per frame by RaytracingRenderer alongside the TLAS (same
// order as the instance descs). Must match the C++ mirror in
// Renderer/ReflectionRenderer.h byte-for-byte (structured buffer - tight
// packing, no cbuffer 16-byte rules).
struct ReflectionInstanceData
{
	// Bindless SRV of the mesh's StructuredBuffer<CompressedVertex> (Crister
	// compressed vertex buffer - 16 bytes, see Model/Crister/CristerGeometry.h CompressedVertex).
	uint vertex_buffer_index_;

	// Bindless SRV of the mesh's flat 32-bit triangle index buffer
	// (PrimitiveIndex() * 3 + n -> vertex index).
	uint index_buffer_index_;

	// Bindless SRV of StructuredBuffer<ReflectionMaterial> - the mesh's full
	// material list (Crister::Materials(), or a single white fallback entry if
	// the mesh has none), indexed via triangle_material_index_buffer_index_
	// below rather than a flat per-instance color. A multi-material mesh (e.g.
	// a Cornell box modeled as one Crister with a red/green/white wall per
	// submesh) needs this - using only materials()[0] for the whole instance
	// was the bug that made every ray-traced bounce off such a mesh come back
	// the same single colour regardless of which wall it actually hit,
	// silently killing color bleeding.
	uint material_data_index_;

	// Bindless SRV of StructuredBuffer<uint>, one entry per triangle in the
	// flat index buffer above, giving that triangle's index into
	// material_data_index_'s array (built from each SubMesh's materialIndex_
	// over its own [indexOffset_/3, (indexOffset_+indexCount_)/3) triangle
	// range - see BuildReflectionMaterialTable).
	uint triangle_material_index_buffer_index_;

	// The compressed vertex UVs are UNORM within this AABB, so decoding them
	// needs the per-mesh bounds (Crister::TexcoordMin / TexcoordExtent) - the
	// same values Model.hlsli gets through its instance data.
	float2 texcoord_min_;
	float2 texcoord_extent_;
};

// Resolves the actual hit triangle's material via the two-step per-triangle
// lookup above. primitive_index is PrimitiveIndex() from the closesthit shader.
ReflectionMaterial ResolveReflectionMaterial(ReflectionInstanceData instance, uint primitive_index)
{
	StructuredBuffer<uint> triangle_material_index = ResourceDescriptorHeap[instance.triangle_material_index_buffer_index_];
	uint material_index = triangle_material_index[primitive_index];

	StructuredBuffer<ReflectionMaterial> materials = ResourceDescriptorHeap[instance.material_data_index_];
	return materials[material_index];
}

// Fully resolved material at one point of the G-Buffer surface, ready for
// EvalDirectLightDispatch: weather (wetness, puddles, snow) and every KHR
// material extension (specular, iridescence, transmission, clearcoat,
// anisotropy, sheen) are already folded in. Shared by every pass that lights
// the G-Buffer surface, so a light's response is identical between them.
struct SurfaceMaterial
{
	float3 diffuse_color_;
	float3 f0_;
	float roughness_;

	float clearcoat_factor_;
	float clearcoat_roughness_;
	float3 clearcoat_normal_;

	float3 sheen_color_;
	float sheen_roughness_;

	float3 anisotropy_tangent_;
	float3 anisotropy_bitangent_;
	float anisotropy_strength_;
};

// Builds a SurfaceMaterial from the G-Buffer values (SurfaceBaseColor,
// SurfaceNormalRoughness), the surface's ModelStructuredBuffer and its
// VisibilityTexcoord. tangent_bits is SurfaceTangent (PackTangentAngle's 16
// bits), which rebuilds the tangent basis for the clearcoat normal map and the
// anisotropy direction. wetness and snow_coverage are the weather's global
// amounts. texcoord_dx/texcoord_dy are the texcoord's screen derivatives,
// which pick the mip of every extension texture.
//
// Weather reshapes the base surface first, so the analytic terms and every
// extension read the weathered surface: rain only wets faces that catch it
// (weighted by normal.y), standing water further needs near-flat ground, and
// snow covers upward faces. Roughness then stays at or above 0.045, because
// the GGX peak 1 / (PI * alpha^2) with alpha = roughness^2 grows with the
// fourth power as roughness falls and this floor keeps it inside half-float
// range. A wet film raises grazing-angle reflectance and a puddle raises it
// further, toward water.
SurfaceMaterial LoadSurfaceMaterial(float3 base_color, float metallic, float roughness, float3 normal, float3 view, ModelStructuredBuffer instance, float2 texcoord, float2 texcoord_dx, float2 texcoord_dy, uint tangent_bits, float wetness, float snow_coverage)
{
	float up_facing = saturate(normal.y);
	float surface_wetness = wetness * up_facing;
	roughness = lerp(roughness, roughness * 0.25, surface_wetness);
	base_color = lerp(base_color, base_color * 0.8, surface_wetness * 0.6);

	float puddle = surface_wetness * saturate((normal.y - 0.85) / 0.15);
	roughness = lerp(roughness, 0.03, puddle);
	base_color = lerp(base_color, base_color * 0.5, puddle * 0.5);

	float snow = snow_coverage * up_facing;
	base_color = lerp(base_color, float3(0.95, 0.96, 1.0), snow);
	roughness = lerp(roughness, 0.9, snow);
	roughness = max(roughness, 0.045);

	float tangent_angle = tangent_bits / 65535.0;

	float specular_factor = instance.extension_.specular_factor_;
	if (instance.extension_.specular_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> specular_texture = ResourceDescriptorHeap[instance.extension_.specular_texture_index_];
		specular_factor *= specular_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).a;
	}

	float3 specular_color = instance.extension_.specular_color_;
	if (instance.extension_.specular_color_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> specular_color_texture = ResourceDescriptorHeap[instance.extension_.specular_color_texture_index_];
		specular_color *= specular_color_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).rgb;
	}

	float iridescence_factor = instance.extension_.iridescence_factor_;
	if (instance.extension_.iridescence_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> iridescence_texture = ResourceDescriptorHeap[instance.extension_.iridescence_texture_index_];
		iridescence_factor *= iridescence_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).r;
	}

	float iridescence_thickness = instance.extension_.iridescence_thickness_;
	if (instance.extension_.iridescence_thickness_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> iridescence_thickness_texture = ResourceDescriptorHeap[instance.extension_.iridescence_thickness_texture_index_];
		iridescence_thickness *= iridescence_thickness_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).g;
	}

	float transmission_factor = instance.extension_.transmission_factor_;
	if (instance.extension_.transmission_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> transmission_texture = ResourceDescriptorHeap[instance.extension_.transmission_texture_index_];
		transmission_factor *= transmission_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).r;
	}

	// KHR_materials_ior/specular: the dielectric F0 follows from the IOR;
	// metals use the base color instead.
	float dielectric = (instance.texture_.ior_ - 1.0) / (instance.texture_.ior_ + 1.0);
	dielectric *= dielectric;
	float3 dielectric_f0 = saturate(dielectric * specular_color * specular_factor);

	// KHR_materials_iridescence: an approximation (a phase shift driven by
	// thickness and view angle, not thin-film interference) that tints F0.
	if (iridescence_factor > 0.0)
	{
		float phase = iridescence_thickness * 0.01 * saturate(dot(normal, view)) + instance.extension_.iridescence_ior_;
		float3 iridescence_shift = sin(float3(phase, phase + 2.094395, phase + 4.18879)) * 0.5 + 0.5;
		dielectric_f0 = lerp(dielectric_f0, iridescence_shift, saturate(iridescence_factor));
	}

	// KHR_materials_transmission: transmitted light does not reflect
	// diffusely; the see-through refraction itself is the Refraction feature's.
	float3 diffuse_color = base_color * (1.0 - metallic) * (1.0 - transmission_factor);
	float3 f0 = lerp(dielectric_f0, base_color, metallic);
	f0 = lerp(f0, max(f0, 0.02), surface_wetness);
	f0 = lerp(f0, max(f0, 0.05), puddle);

	float clearcoat_factor = instance.extension_.clearcoat_factor_;
	if (instance.extension_.clearcoat_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> clearcoat_texture = ResourceDescriptorHeap[instance.extension_.clearcoat_texture_index_];
		clearcoat_factor *= clearcoat_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).r;
	}

	float clearcoat_roughness = instance.extension_.clearcoat_roughness_;
	if (instance.extension_.clearcoat_roughness_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> clearcoat_roughness_texture = ResourceDescriptorHeap[instance.extension_.clearcoat_roughness_texture_index_];
		clearcoat_roughness *= clearcoat_roughness_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).g;
	}

	// KHR_materials_clearcoat: the coat carries its own normal map in the
	// surface's tangent basis; only the coat normal changes, never the base.
	float3 clearcoat_normal = normal;
	if (instance.extension_.clearcoat_normal_texture_index_ != MODEL_INVALID)
	{
		float3 clearcoat_tangent;
		float clearcoat_handedness;
		UnpackTangentAngle(normal, tangent_angle, clearcoat_tangent, clearcoat_handedness);
		float3 clearcoat_bitangent = cross(normal, clearcoat_tangent) * clearcoat_handedness;

		Texture2D<float4> clearcoat_normal_texture = ResourceDescriptorHeap[instance.extension_.clearcoat_normal_texture_index_];
		float3 clearcoat_normal_sample = clearcoat_normal_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).xyz * 2.0 - 1.0;
		clearcoat_normal = normalize(mul(clearcoat_normal_sample, float3x3(clearcoat_tangent, clearcoat_bitangent, normal)));
	}

	// KHR_materials_anisotropy: the direction is a tangent-space 2D vector made
	// from anisotropyRotation and anisotropyTexture.rg; the strength is
	// anisotropyStrength times anisotropyTexture.b.
	float3 anisotropy_tangent = float3(1.0, 0.0, 0.0);
	float3 anisotropy_bitangent = float3(0.0, 1.0, 0.0);
	float anisotropy_strength = instance.extension_.anisotropy_;
	if (abs(anisotropy_strength) > 0.0)
	{
		float2 anisotropy_direction = float2(cos(instance.extension_.anisotropy_rotation_), sin(instance.extension_.anisotropy_rotation_));
		if (instance.extension_.anisotropy_texture_index_ != MODEL_INVALID)
		{
			Texture2D<float4> anisotropy_texture = ResourceDescriptorHeap[instance.extension_.anisotropy_texture_index_];
			float3 anisotropy_sample = anisotropy_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).rgb;
			float2 texture_direction = anisotropy_sample.rg * 2.0 - 1.0;
			anisotropy_direction = float2(texture_direction.x * anisotropy_direction.x - texture_direction.y * anisotropy_direction.y, texture_direction.x * anisotropy_direction.y + texture_direction.y * anisotropy_direction.x);
			anisotropy_strength *= anisotropy_sample.b;
		}

		float3 base_tangent;
		float base_handedness;
		UnpackTangentAngle(normal, tangent_angle, base_tangent, base_handedness);
		float3 base_bitangent = cross(normal, base_tangent) * base_handedness;
		anisotropy_tangent = normalize(base_tangent * anisotropy_direction.x + base_bitangent * anisotropy_direction.y);
		anisotropy_bitangent = normalize(cross(normal, anisotropy_tangent));
	}

	float3 sheen_color = instance.extension_.sheen_color_;
	if (instance.extension_.sheen_color_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> sheen_color_texture = ResourceDescriptorHeap[instance.extension_.sheen_color_texture_index_];
		sheen_color *= sheen_color_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).rgb;
	}

	float sheen_roughness = instance.extension_.sheen_roughness_;
	if (instance.extension_.sheen_roughness_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> sheen_roughness_texture = ResourceDescriptorHeap[instance.extension_.sheen_roughness_texture_index_];
		sheen_roughness *= sheen_roughness_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).a;
	}

	SurfaceMaterial result;
	result.diffuse_color_ = diffuse_color;
	result.f0_ = f0;
	result.roughness_ = roughness;
	result.clearcoat_factor_ = clearcoat_factor;
	result.clearcoat_roughness_ = clearcoat_roughness;
	result.clearcoat_normal_ = clearcoat_normal;
	result.sheen_color_ = sheen_color;
	result.sheen_roughness_ = max(sheen_roughness, 0.001);
	result.anisotropy_tangent_ = anisotropy_tangent;
	result.anisotropy_bitangent_ = anisotropy_bitangent;
	result.anisotropy_strength_ = anisotropy_strength;
	return result;
}

/**
* True when a ray should pass THROUGH this candidate triangle: OPAQUE always
* blocks, MASK blocks only where base color alpha reaches alphaCutoff, BLEND
* never blocks (the PPLL in Model/Transparent draws those surfaces instead).
* Only reached on meshes whose BLAS geometry was declared non-opaque, which
* RaytracingRenderer does whenever any of the mesh's materials is MASK or
* BLEND (see geometryDesc.opaque_); a fully OPAQUE mesh keeps
* D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE and never reaches here.
* instance_data_index is shader_resource_indices.raytracing_.instance_data_index_.
* Shared by the reflection,
* refraction, ambient-occlusion, subsurface-scattering and global-illumination
* ray paths.
*/
bool IsMaterialPassthrough(uint instance_data_index, uint instance_id, uint primitive_index, float2 barycentrics, float3x4 object_to_world, float3x4 world_to_object, float3 world_position)
{
	StructuredBuffer<ReflectionInstanceData> instances = ResourceDescriptorHeap[instance_data_index];
	ReflectionInstanceData instance = instances[instance_id];

	ReflectionMaterial material = ResolveReflectionMaterial(instance, primitive_index);

	/// alpha_mode_: 0 = OPAQUE, 1 = MASK, 2 = BLEND (glTF convention).
	if (material.alpha_mode_ == 0)
	{
		return false;
	}

	if (material.alpha_mode_ == 2)
	{
		return true;
	}

	float alpha = material.base_color_alpha_;

	if (material.base_color_texture_index_ != 0xFFFFFFFF)
	{
		StructuredBuffer<uint> triangle_indices = ResourceDescriptorHeap[instance.index_buffer_index_];
		StructuredBuffer<CompressedVertex> vertices = ResourceDescriptorHeap[instance.vertex_buffer_index_];

		uint base_index = primitive_index * 3;
		CompressedVertex vertex0 = vertices[triangle_indices[base_index + 0]];
		CompressedVertex vertex1 = vertices[triangle_indices[base_index + 1]];
		CompressedVertex vertex2 = vertices[triangle_indices[base_index + 2]];

		float weight0 = 1.0 - barycentrics.x - barycentrics.y;
		float weight1 = barycentrics.x;
		float weight2 = barycentrics.y;

		float2 texcoord =
			DecodeCompressedVertexTexcoord(vertex0, instance.texcoord_min_, instance.texcoord_extent_) * weight0 +
			DecodeCompressedVertexTexcoord(vertex1, instance.texcoord_min_, instance.texcoord_extent_) * weight1 +
			DecodeCompressedVertexTexcoord(vertex2, instance.texcoord_min_, instance.texcoord_extent_) * weight2;

		MaterialMappingCoordinate mapping;
		mapping.texcoord_x_ = texcoord;
		mapping.texcoord_y_ = texcoord;
		mapping.texcoord_z_ = texcoord;
		mapping.weight_ = float3(1.0, 0.0, 0.0);
		mapping.projection_normal_ = float3(0.0, 0.0, 1.0);
		if (material.material_mapping_ != MATERIAL_MAPPING_STRETCH)
		{
			float3 object_normal = normalize(DecodeCompressedVertexNormal(vertex0) * weight0 + DecodeCompressedVertexNormal(vertex1) * weight1 + DecodeCompressedVertexNormal(vertex2) * weight2);
			float4 object_tangent = DecodeCompressedVertexTangent(vertex0) * weight0 + DecodeCompressedVertexTangent(vertex1) * weight1 + DecodeCompressedVertexTangent(vertex2) * weight2;
			float3 object_bitangent = cross(object_normal, object_tangent.xyz) * DecodeCompressedVertexTangent(vertex0).w;
			float3 object_position = mul(world_to_object, float4(world_position, 1.0));
			float3 object_scale = float3(length(float3(object_to_world[0][0], object_to_world[1][0], object_to_world[2][0])), length(float3(object_to_world[0][1], object_to_world[1][1], object_to_world[2][1])), length(float3(object_to_world[0][2], object_to_world[1][2], object_to_world[2][2])));
			float3 world_normal = normalize(mul((float3x3)object_to_world, object_normal));
			mapping = ResolveMaterialMapping(material.material_mapping_, material.tiling_, material.triplanar_sharpness_, texcoord, object_position, object_normal, object_tangent.xyz, object_bitangent, object_scale, world_position, world_normal);
		}

		Texture2D<float4> base_color_texture = ResourceDescriptorHeap[material.base_color_texture_index_];
		alpha *= SampleMaterialMappingLevel(base_color_texture, sampler_linear_wrap, mapping, 0).a;
	}

	return alpha < material.alpha_cutoff_;
}

#endif // __MATERIAL_HLSL__
