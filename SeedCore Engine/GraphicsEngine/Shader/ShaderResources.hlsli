#ifndef __SHADER_RESOURCES_HLSL__
#define __SHADER_RESOURCES_HLSL__

#include "Denoiser.hlsli"
#include "../Model/Opaque/GeometryBuffer.hlsli"
#include "../PostProcess/PostProcess.hlsli"
#include "../Light/Cluster.hlsli"
#include "../Shape/HUD/HUD.hlsli"
#include "../Shape/Primitive/Solid/PrimitiveSolid.hlsli"
#include "../Shape/Primitive/Wireframe/PrimitiveWireframe.hlsli"
#include "../Texture/Texture.hlsli"
#include "../Font/Font.hlsli"
#include "../Movie/Movie.hlsli"
#include "../Model/Model.hlsli"
#include "../Raytracing/Raytracing.hlsli"
#include "../Illumination/Shadow/Raytracing/RaytracingShadow.hlsli"
#include "../Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusion.hlsli"
#include "../Illumination/Reflection/Raytracing/RaytracingReflectionReSTIR.hlsli"
#include "../Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationReSTIR.hlsli"
#include "../Illumination/Refraction/Refraction.hlsli"
#include "../Illumination/SubsurfaceScattering/SubsurfaceScattering.hlsli"
#include "../Illumination/VolumetricLight/VolumetricLight.hlsli"
#include "../Environment/VolumetricCloud/VolumetricCloudScapes.hlsli"
#include "../Environment/VolumetricStar/VolumetricStar.hlsli"
#include "../Sky/Sky.hlsli"
#include "../Environment/WeatherParticle.hlsli"

struct ShaderResourceIndices
{
	LightShaderResourceIndices light_;
	ClusterAssignShaderResourceIndices cluster_assign_;
	RaytracingShadowAccumulationShaderResourceIndices shadow_accumulation_;
	RaytracingAmbientOcclusionAccumulationShaderResourceIndices ambient_occlusion_accumulation_;
	RaytracingGlobalIlluminationAccumulationShaderResourceIndices global_illumination_accumulation_;
	RaytracingReflectionAccumulationShaderResourceIndices reflection_accumulation_;

	PostProcessShaderResourceIndices post_process_;
	HUDShaderResourceIndices hud_;
	PrimitiveSolidShaderResourceIndices primitive_solid_;
	PrimitiveWireframeShaderResourceIndices primitive_wireframe_;

	TextureShaderResourceIndices texture_;
	FontShaderResourceIndices font_;
	MovieShaderResourceIndices movie_;
	ModelShaderResourceIndices model_;
	GeometryBufferShaderResourceIndices geometry_buffer_;
	SkyShaderResourceIndices sky_;
	RaytracingShaderResourceIndices raytracing_;
	RaytracingShadowShaderResourceIndices shadow_;
	RaytracingAmbientOcclusionShaderResourceIndices ambient_occlusion_;
	SubsurfaceScatteringShaderResourceIndices subsurface_scattering_;
	RaytracingReflectionShaderResourceIndices reflection_;
	RefractionShaderResourceIndices refraction_;
	RaytracingGlobalIlluminationShaderResourceIndices global_illumination_;
	CloudShaderResourceIndices cloud_;
	StarShaderResourceIndices star_;
	WeatherParticleShaderResourceIndices weather_particle_;
	VolumetricLightShaderResourceIndices volumetric_light_;
};
ConstantBuffer<ShaderResourceIndices> shader_resource_indices : register(b0, space1);

#endif // __SHADER_RESOURCES_HLSL__
