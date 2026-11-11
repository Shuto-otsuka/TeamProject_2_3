#pragma once
#include <GraphicsEngine/Sky/SkyFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>

namespace SeedCore
{
	struct SkyFeature::GenerateEquirectStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool SkyFeature::* enableCondition_ = &SkyFeature::generate_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Write(Cubemap)
		};

		static void Execute(SkyFeature& feature, RenderPass& pass);
	};

	struct SkyFeature::DiffuseIrradianceStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool SkyFeature::* enableCondition_ = &SkyFeature::generate_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Cubemap),
			RenderAccess::Write(DiffuseIrradiance)
		};

		static void Execute(SkyFeature& feature, RenderPass& pass);
	};

	struct SkyFeature::SpecularPrefilterStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Frame;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Frame;

		SC_CONSTEX Bool SkyFeature::* enableCondition_ = &SkyFeature::generate_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Cubemap),
			RenderAccess::Write(SpecularPrefiltered)
		};

		static void Execute(SkyFeature& feature, RenderPass& pass);
	};

	struct SkyFeature::BrdfLutStage :RenderStage
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

		SC_CONSTEX Bool SkyFeature::* enableCondition_ = &SkyFeature::resolving_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(Cubemap),
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Write(Radiance)
		};

		static void Execute(SkyFeature& feature, RenderPass& pass);
	};
}
