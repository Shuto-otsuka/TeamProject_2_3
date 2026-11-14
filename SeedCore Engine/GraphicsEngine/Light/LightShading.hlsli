#ifndef __LIGHT_SHADING_HLSL__
#define __LIGHT_SHADING_HLSL__

#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Model/Opaque/PbrShading.hlsli>
#include <GraphicsEngine/Model/Opaque/PhongShading.hlsli>
#include <GraphicsEngine/Model/Opaque/ToonShading.hlsli>
#include <GraphicsEngine/Model/Opaque/FurShading.hlsli>

// Picks the per-shading-model direct light response for one light. Shared by
// the deferred lighting of the primary surface and the ray-traced punctual
// light estimate, so a punctual light's specular response is identical
// between the two; only "how to shade against one direct light" lives here.
float3 EvalDirectLightDispatch(uint shading_model, float3 normal, float3 view, float3 light_direction, float3 diffuse_color, float3 f0, float roughness, float clearcoat, float clearcoat_roughness, float3 clearcoat_normal, float3 sheen_color, float sheen_roughness, float3 anisotropy_tangent, float3 anisotropy_bitangent, float anisotropy_strength)
{
	const float PI = 3.14159265358979;

	if (shading_model == SHADING_MODEL_PHONG)
	{
		float phong_alpha = max(roughness * roughness, 0.01);
		float shininess = max(0.5 / (phong_alpha * phong_alpha) - 0.5, 1.0);
		return EvalDirectLightPhong(normal, view, light_direction, diffuse_color, f0, shininess);
	}

	if (shading_model == SHADING_MODEL_TOON)
	{
		float shininess = lerp(64.0, 4.0, roughness);
		return EvalDirectLightToon(normal, view, light_direction, diffuse_color, f0, shininess);
	}

	if (shading_model == SHADING_MODEL_LAMBERT)
	{
		return diffuse_color / PI * saturate(dot(normal, light_direction));
	}

	if (shading_model == SHADING_MODEL_FUR)
	{
		float fur_shininess = lerp(48.0, 4.0, roughness);
		return EvalDirectLightFur(normal, anisotropy_tangent, view, light_direction, diffuse_color, f0, fur_shininess);
	}

	return EvalDirectLight(normal, view, light_direction, diffuse_color, f0, roughness, clearcoat, clearcoat_roughness, clearcoat_normal, sheen_color, sheen_roughness, anisotropy_tangent, anisotropy_bitangent, anisotropy_strength);
}

#endif // __LIGHT_SHADING_HLSL__
