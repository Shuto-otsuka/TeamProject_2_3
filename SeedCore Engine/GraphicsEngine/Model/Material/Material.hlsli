#ifndef __MATERIAL_HLSL__
#define __MATERIAL_HLSL__

#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Model/Material/MaterialMapping.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>
#include <GraphicsEngine/Shader/Vertex.hlsli>

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
		Texture2D<float4> specular_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.extension_.specular_texture_index_)];
		specular_factor *= specular_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).a;
	}

	float3 specular_color = instance.extension_.specular_color_;
	if (instance.extension_.specular_color_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> specular_color_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.extension_.specular_color_texture_index_)];
		specular_color *= specular_color_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).rgb;
	}

	float iridescence_factor = instance.extension_.iridescence_factor_;
	if (instance.extension_.iridescence_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> iridescence_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.extension_.iridescence_texture_index_)];
		iridescence_factor *= iridescence_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).r;
	}

	float iridescence_thickness = instance.extension_.iridescence_thickness_;
	if (instance.extension_.iridescence_thickness_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> iridescence_thickness_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.extension_.iridescence_thickness_texture_index_)];
		iridescence_thickness *= iridescence_thickness_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).g;
	}

	float transmission_factor = instance.extension_.transmission_factor_;
	if (instance.extension_.transmission_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> transmission_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.extension_.transmission_texture_index_)];
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
		Texture2D<float4> clearcoat_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.extension_.clearcoat_texture_index_)];
		clearcoat_factor *= clearcoat_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).r;
	}

	float clearcoat_roughness = instance.extension_.clearcoat_roughness_;
	if (instance.extension_.clearcoat_roughness_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> clearcoat_roughness_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.extension_.clearcoat_roughness_texture_index_)];
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

		Texture2D<float4> clearcoat_normal_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.extension_.clearcoat_normal_texture_index_)];
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
			Texture2D<float4> anisotropy_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.extension_.anisotropy_texture_index_)];
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
		Texture2D<float4> sheen_color_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.extension_.sheen_color_texture_index_)];
		sheen_color *= sheen_color_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).rgb;
	}

	float sheen_roughness = instance.extension_.sheen_roughness_;
	if (instance.extension_.sheen_roughness_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> sheen_roughness_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.extension_.sheen_roughness_texture_index_)];
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

#endif // __MATERIAL_HLSL__
