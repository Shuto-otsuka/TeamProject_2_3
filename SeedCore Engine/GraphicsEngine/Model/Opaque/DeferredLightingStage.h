#pragma once
#include <GraphicsEngine/Model/Opaque/DeferredLightingFeature.h>
#include <GraphicsEngine/Illumination/DirectLight/DirectLightFeature.h>
#include <GraphicsEngine/Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusionFeature.h>
#include <GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationFeature.h>
#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionFeature.h>
#include <GraphicsEngine/Illumination/Refraction/Raytracing/RaytracingRefractionFeature.h>
#include <GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadowFeature.h>
#include <GraphicsEngine/Illumination/SubsurfaceScattering/Raytracing/RaytracingSubsurfaceScatteringFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Sky/SkyFeature.h>
#include <GraphicsEngine/Environment/Weather/WeatherFeature.h>

namespace SeedCore
{
	struct DeferredLightingFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Lighting;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::BaseColor),
			RenderAccess::Read(MaterialFeature::NormalRoughness),
			RenderAccess::Read(MaterialFeature::Tangent),
			RenderAccess::Read(GeometryFeature::VisibilityID),
			RenderAccess::Read(GeometryFeature::VisibilityUV),
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Constant(WeatherFeature::Constants),
			RenderAccess::Read(RaytracingShadowFeature::Visibility),
			RenderAccess::Read(RaytracingAmbientOcclusionFeature::Openness),
			RenderAccess::Read(RaytracingSubsurfaceScatteringFeature::Transmittance),
			RenderAccess::Read(RaytracingReflectionFeature::Radiance),
			RenderAccess::Read(RaytracingGlobalIlluminationFeature::Radiance),
			RenderAccess::Read(RaytracingRefractionFeature::Radiance),
			RenderAccess::Read(DirectLightFeature::Radiance),
			RenderAccess::Read(SkyFeature::Diffuse),
			RenderAccess::Read(SkyFeature::Specular),
			RenderAccess::Read(SkyFeature::BRDF),
			RenderAccess::Target(MaterialFeature::Radiance)
		};

		static void Execute(DeferredLightingFeature& feature, RenderPass& pass);
	};
}
