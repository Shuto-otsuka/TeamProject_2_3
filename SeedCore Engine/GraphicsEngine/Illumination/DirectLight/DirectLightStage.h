#pragma once
#include <GraphicsEngine/Illumination/DirectLight/DirectLightFeature.h>
#include <GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadowFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Environment/Weather/WeatherFeature.h>

namespace SeedCore
{
	struct DirectLightFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Illumination;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::DirectLight);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(MaterialFeature::BaseColor),
			RenderAccess::Read(MaterialFeature::NormalRoughness),
			RenderAccess::Read(MaterialFeature::Tangent),
			RenderAccess::Read(GeometryFeature::VisibilityID),
			RenderAccess::Read(GeometryFeature::VisibilityUV),
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(LightFeature::List),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Constant(WeatherFeature::Constants),
			RenderAccess::Read(LightFeature::Clusters),
			RenderAccess::Read(RaytracingShadowFeature::Visibility),
			RenderAccess::Write(Radiance)
		};

		static void Execute(DirectLightFeature& feature, RenderPass& pass);
	};
}
