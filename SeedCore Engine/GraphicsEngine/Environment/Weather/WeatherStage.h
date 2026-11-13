#pragma once
#include <GraphicsEngine/Environment/Weather/WeatherFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Illumination/Radiance/RadianceFeature.h>
#include <GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLightFeature.h>

namespace SeedCore
{
	struct WeatherFeature::UpdateStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX Bool WeatherFeature::* enableCondition_ = &WeatherFeature::falling_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Emitter),
			RenderAccess::Write(Particles)
		};

		static void Execute(WeatherFeature& feature, RenderPass& pass);
	};

	struct WeatherFeature::DrawStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Background;

		SC_CONSTEX Bool WeatherFeature::* enableCondition_ = &WeatherFeature::falling_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Weather);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Particles),
			RenderAccess::Constant(Emitter),
			RenderAccess::Read(RaytracingVolumetricLightFeature::FogVolume),
			RenderAccess::Target(RadianceFeature::Color),
			RenderAccess::DepthTest(GeometryFeature::Depth)
		};

		static void Execute(WeatherFeature& feature, RenderPass& pass);
	};
}
