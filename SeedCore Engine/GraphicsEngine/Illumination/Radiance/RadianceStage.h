#pragma once
#include <GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLightFeature.h>
#include <GraphicsEngine/Illumination/Radiance/RadianceFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Sky/SkyFeature.h>

namespace SeedCore
{
	struct RadianceFeature::ComposeStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Compose;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(SkyFeature::Radiance),
			RenderAccess::Read(MaterialFeature::Radiance),
			RenderAccess::Read(RaytracingVolumetricLightFeature::Fog),
			RenderAccess::Write(Color)
		};

		static void Execute(RadianceFeature& feature, RenderPass& pass);
	};
}
