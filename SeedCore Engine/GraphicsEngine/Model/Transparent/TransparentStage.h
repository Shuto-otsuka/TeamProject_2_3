#pragma once
#include <GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLightFeature.h>
#include <GraphicsEngine/Model/Transparent/TransparentFeature.h>
#include <GraphicsEngine/Model/ModelFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/PostProcess/Outline/OutlineFeature.h>
#include <GraphicsEngine/Illumination/Radiance/RadianceFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>
#include <GraphicsEngine/Raytracing/TopLevelFeature.h>
#include <GraphicsEngine/Sky/SkyFeature.h>
#include <GraphicsEngine/Environment/Weather/WeatherFeature.h>

namespace SeedCore
{
	struct TransparentFeature::UpdateStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Transparent;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Transparent);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(Heads),
			RenderAccess::Write(Counter)
		};

		static void Execute(TransparentFeature& feature, RenderPass& pass);
	};

	struct TransparentFeature::DrawStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Raster;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Transparent;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Transparent);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Instances),
			RenderAccess::Read(CullingFeature::TransparentMeshlets),
			RenderAccess::Read(CullingFeature::TransparentArguments),
			RenderAccess::Read(CullingFeature::TransparentChunks),
			RenderAccess::Read(GeometryFeature::HiZ),
			RenderAccess::Read(ModelFeature::Bones),
			RenderAccess::Read(ModelFeature::Morphs),
			RenderAccess::Read(LightFeature::List),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Constant(WeatherFeature::Constants),
			RenderAccess::Read(LightFeature::Clusters),
			RenderAccess::Read(SkyFeature::Diffuse),
			RenderAccess::Read(SkyFeature::Specular),
			RenderAccess::Read(SkyFeature::BRDF),
			RenderAccess::Read(TopLevelFeature::Structure),
			RenderAccess::Read(TopLevelFeature::Hits),
			RenderAccess::Write(Heads),
			RenderAccess::Write(Fragments),
			RenderAccess::Write(Counter),
			RenderAccess::Read(OutlineFeature::Styles),
			RenderAccess::Write(OutlineFeature::Mask),
			RenderAccess::DepthTest(GeometryFeature::Depth)
		};

		static void Execute(TransparentFeature& feature, RenderPass& pass);
	};

	struct TransparentFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Transparent;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Transparent);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Heads),
			RenderAccess::Read(Fragments),
			RenderAccess::Read(RaytracingVolumetricLightFeature::Fog),
			RenderAccess::Write(RadianceFeature::Color),
			RenderAccess::Write(MaterialFeature::Velocity)
		};

		static void Execute(TransparentFeature& feature, RenderPass& pass);
	};
}
