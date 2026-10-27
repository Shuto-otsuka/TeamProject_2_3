#ifndef __UNORDERED_ACCESSES_HLSL__
#define __UNORDERED_ACCESSES_HLSL__

#include "Denoiser.hlsli"
#include "../Model/Opaque/GeometryBuffer.hlsli"
#include "Material.hlsli"
#include "../PostProcess/PostProcess.hlsli"
#include "../DLSS/Dlss.hlsli"
#include "../Light/Cluster.hlsli"
#include "../Model/Model.hlsli"
#include "../Illumination/Shadow/Raytracing/RaytracingShadow.hlsli"
#include "../Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusion.hlsli"
#include "../Illumination/SubsurfaceScattering/SubsurfaceScattering.hlsli"
#include "../Illumination/Reflection/Raytracing/RaytracingReflectionReSTIR.hlsli"
#include "../Illumination/Refraction/Refraction.hlsli"
#include "../Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationReSTIR.hlsli"
#include "../Environment/VolumetricCloud/VolumetricCloudScapes.hlsli"
#include "../Environment/VolumetricStar/VolumetricStar.hlsli"
#include "../Environment/WeatherParticle.hlsli"
#include "../Illumination/VolumetricLight/VolumetricLight.hlsli"

struct UnorderedAccessIndices
{
	ClusterAssignUnorderedAccessIndices cluster_assign_;
	RaytracingShadowAccumulationUnorderedAccessIndices shadow_accumulation_;
	RaytracingAmbientOcclusionAccumulationUnorderedAccessIndices ambient_occlusion_accumulation_;
	RaytracingGlobalIlluminationAccumulationUnorderedAccessIndices global_illumination_accumulation_;
	RaytracingReflectionAccumulationUnorderedAccessIndices reflection_accumulation_;
	DlssUnorderedAccessIndices dlss_;

	PostProcessUnorderedAccessIndices post_process_;

	OitUnorderedAccessIndices oit_;
	GeometryBufferUnorderedAccessIndices geometry_buffer_;
	MaterialSortUnorderedAccessIndices material_sort_;
	RaytracingShadowUnorderedAccessIndices shadow_;
	RaytracingAmbientOcclusionUnorderedAccessIndices ambient_occlusion_;
	SubsurfaceScatteringUnorderedAccessIndices subsurface_scattering_;
	RaytracingReflectionUnorderedAccessIndices reflection_;
	RefractionUnorderedAccessIndices refraction_;
	RaytracingGlobalIlluminationUnorderedAccessIndices global_illumination_;
	CloudUnorderedAccessIndices cloud_;
	StarUnorderedAccessIndices star_;
	WeatherParticleUnorderedAccessIndices weather_particle_;
	VolumetricLightUnorderedAccessIndices volumetric_light_;
};
ConstantBuffer<UnorderedAccessIndices> unordered_access_indices : register(b1, space1);

#endif // __UNORDERED_ACCESSES_HLSL__
