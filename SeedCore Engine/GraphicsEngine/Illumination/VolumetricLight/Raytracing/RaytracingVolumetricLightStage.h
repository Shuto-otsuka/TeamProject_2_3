#pragma once
#include <GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLightFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelFeature.h>
#include <GraphicsEngine/Sky/SkyFeature.h>

namespace SeedCore
{
	struct RaytracingVolumetricLightFeature::BuildStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingVolumetricLightFeature::* enableCondition_ = &RaytracingVolumetricLightFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::VolumetricLight);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Density)
		};

		static void Execute(RaytracingVolumetricLightFeature& feature, RenderPass& pass);
	};

	struct RaytracingVolumetricLightFeature::TraceStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Bool RaytracingVolumetricLightFeature::* enableCondition_ = &RaytracingVolumetricLightFeature::tracing_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::VolumetricLight);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Density),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Read(SkyFeature::Diffuse),
			RenderAccess::Read(TopLevelFeature::Structure),
			RenderAccess::Read(TopLevelFeature::Hits),
			RenderAccess::Constant(Constants),
			RenderAccess::History(Scattering),
			RenderAccess::Write(Scattering)
		};

		static void Execute(RaytracingVolumetricLightFeature& feature, RenderPass& pass);
	};

	struct RaytracingVolumetricLightFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Scattering),
			RenderAccess::Write(Fog)
		};

		static void Execute(RaytracingVolumetricLightFeature& feature, RenderPass& pass);
	};
}
