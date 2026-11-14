#pragma once
#include <GraphicsEngine/PostProcess/LensFlare/LensFlareFeature.h>
#include <GraphicsEngine/PostProcess/Bokeh/BokehFeature.h>

namespace SeedCore
{
	struct LensFlareFeature::FilterStage
	{
		struct DownsampleStep;

		struct BlurStep;

		using Sequence = RenderSequence<DownsampleStep, BlurStep>;
	};

	struct LensFlareFeature::FilterStage::DownsampleStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool LensFlareFeature::* enableCondition_ = &LensFlareFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::LensFlare);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(BokehFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Bright)
		};

		static void Execute(LensFlareFeature& feature, RenderPass& pass);
	};

	struct LensFlareFeature::FilterStage::BlurStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool LensFlareFeature::* enableCondition_ = &LensFlareFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::LensFlare);

		SC_CONSTEX Uint32 repeat_ = 4;

		SC_CONSTEX RenderAccess accesses_[][3] =
		{
			{
				RenderAccess::Constant(Constants),
				RenderAccess::Read(Bright).As("Source"),
				RenderAccess::Write(Ping).As("Destination")
			},
			{
				RenderAccess::Constant(Constants),
				RenderAccess::Read(Ping).As("Source"),
				RenderAccess::Write(Pong).As("Destination")
			},
			{
				RenderAccess::Constant(Constants),
				RenderAccess::Read(Pong).As("Source"),
				RenderAccess::Write(Ping).As("Destination")
			},
			{
				RenderAccess::Constant(Constants),
				RenderAccess::Read(Ping).As("Source"),
				RenderAccess::Write(Pong).As("Destination")
			}
		};

		static void Execute(LensFlareFeature& feature, RenderPass& pass);
	};

	struct LensFlareFeature::ComposeStage
	{
		struct GlareStep;

		struct GhostStep;

		using Sequence = RenderSequence<GlareStep, GhostStep>;
	};

	struct LensFlareFeature::ComposeStage::GlareStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool LensFlareFeature::* enableCondition_ = &LensFlareFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::LensFlare);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Pong),
			RenderAccess::Write(Radiance)
		};

		static void Execute(LensFlareFeature& feature, RenderPass& pass);
	};

	struct LensFlareFeature::ComposeStage::GhostStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool LensFlareFeature::* enableCondition_ = &LensFlareFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::LensFlare);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Bright),
			RenderAccess::Write(Radiance)
		};

		static void Execute(LensFlareFeature& feature, RenderPass& pass);
	};
}
