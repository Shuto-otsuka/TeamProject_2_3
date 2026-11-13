#pragma once
#include <GraphicsEngine/Sky/SkyFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Light/LightFeature.h>
#include <GraphicsEngine/Environment/Weather/WeatherFeature.h>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudFeature.h>
#include <GraphicsEngine/Environment/VolumetricStar/VolumetricStarFeature.h>

namespace SeedCore
{
	struct SkyFeature::BuildStage
	{
		struct EquirectStep;

		struct ProceduralStep;

		struct IrradianceStep;

		struct PrefilterStep;

		struct BrdfLutStep;

		using Sequence = RenderSequence<EquirectStep, ProceduralStep, IrradianceStep, PrefilterStep, BrdfLutStep>;
	};

	struct SkyFeature::BuildStage::EquirectStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool SkyFeature::* enableCondition_ = &SkyFeature::equirect_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(Cubemap)
		};

		static void Execute(SkyFeature& feature, RenderPass& pass);
	};

	struct SkyFeature::BuildStage::ProceduralStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool SkyFeature::* enableCondition_ = &SkyFeature::procedural_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(VolumetricCloudFeature::Constants),
			RenderAccess::Read(VolumetricCloudFeature::Shape),
			RenderAccess::Read(VolumetricCloudFeature::Detail),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Write(Cubemap)
		};

		static void Execute(SkyFeature& feature, RenderPass& pass);
	};

	struct SkyFeature::BuildStage::IrradianceStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool SkyFeature::* enableCondition_ = &SkyFeature::filtering_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Cubemap),
			RenderAccess::Write(DiffuseIrradiance)
		};

		static void Execute(SkyFeature& feature, RenderPass& pass);
	};

	struct SkyFeature::BuildStage::PrefilterStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool SkyFeature::* enableCondition_ = &SkyFeature::filtering_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Cubemap),
			RenderAccess::Write(SpecularPrefiltered)
		};

		static void Execute(SkyFeature& feature, RenderPass& pass);
	};

	struct SkyFeature::BuildStage::BrdfLutStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool SkyFeature::* enableCondition_ = &SkyFeature::brdfLut_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(BrdfLut)
		};

		static void Execute(SkyFeature& feature, RenderPass& pass);
	};

	struct SkyFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Lighting;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::Sky);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Cubemap),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Constant(VolumetricCloudFeature::Constants),
			RenderAccess::Read(VolumetricCloudFeature::Radiance),
			RenderAccess::Read(VolumetricStarFeature::Radiance),
			RenderAccess::Constant(WeatherFeature::Constants),
			RenderAccess::Constant(LightFeature::Constants),
			RenderAccess::Write(Radiance)
		};

		static void Execute(SkyFeature& feature, RenderPass& pass);
	};
}
