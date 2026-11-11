#pragma once
#include <GraphicsEngine/PostProcess/LensFlare/LensFlareFeature.h>
#include <GraphicsEngine/PostProcess/Bokeh/BokehFeature.h>

namespace SeedCore
{
	struct LensFlareFeature::DownsampleStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool LensFlareFeature::* enableCondition_ = &LensFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(BokehFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Bright)
		};

		static void Execute(LensFlareFeature& feature, RenderPass& pass);
	};

	struct LensFlareFeature::BlurPass1Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool LensFlareFeature::* enableCondition_ = &LensFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Bright),
			RenderAccess::Write(Ping)
		};

		static void Execute(LensFlareFeature& feature, RenderPass& pass);
	};

	struct LensFlareFeature::BlurPass2Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool LensFlareFeature::* enableCondition_ = &LensFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Ping),
			RenderAccess::Write(Pong)
		};

		static void Execute(LensFlareFeature& feature, RenderPass& pass);
	};

	struct LensFlareFeature::BlurPass3Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool LensFlareFeature::* enableCondition_ = &LensFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Pong),
			RenderAccess::Write(Ping)
		};

		static void Execute(LensFlareFeature& feature, RenderPass& pass);
	};

	struct LensFlareFeature::BlurPass4Stage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool LensFlareFeature::* enableCondition_ = &LensFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Ping),
			RenderAccess::Write(Pong)
		};

		static void Execute(LensFlareFeature& feature, RenderPass& pass);
	};

	struct LensFlareFeature::ComposeStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool LensFlareFeature::* enableCondition_ = &LensFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Pong),
			RenderAccess::Write(Radiance)
		};

		static void Execute(LensFlareFeature& feature, RenderPass& pass);
	};

	struct LensFlareFeature::GhostStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool LensFlareFeature::* enableCondition_ = &LensFlareFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Bright),
			RenderAccess::Write(Radiance)
		};

		static void Execute(LensFlareFeature& feature, RenderPass& pass);
	};
}
