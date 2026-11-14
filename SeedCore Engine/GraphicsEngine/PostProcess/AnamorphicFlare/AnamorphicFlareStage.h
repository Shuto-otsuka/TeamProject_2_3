#pragma once
#include <GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareFeature.h>
#include <GraphicsEngine/PostProcess/Bokeh/BokehFeature.h>

namespace SeedCore
{
	struct AnamorphicFlareFeature::FilterStage
	{
		struct PrefilterStep;

		struct BlurStep;

		using Sequence = RenderSequence<PrefilterStep, BlurStep>;
	};

	struct AnamorphicFlareFeature::FilterStage::PrefilterStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AnamorphicFlareFeature::* enableCondition_ = &AnamorphicFlareFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::AnamorphicFlare);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(BokehFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Ping)
		};

		static void Execute(AnamorphicFlareFeature& feature, RenderPass& pass);
	};

	struct AnamorphicFlareFeature::FilterStage::BlurStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AnamorphicFlareFeature::* enableCondition_ = &AnamorphicFlareFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::AnamorphicFlare);

		SC_CONSTEX Uint32 repeat_ = 4;

		SC_CONSTEX RenderAccess accesses_[][3] =
		{
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
			},
			{
				RenderAccess::Constant(Constants),
				RenderAccess::Read(Pong).As("Source"),
				RenderAccess::Write(Ping).As("Destination")
			}
		};

		static void Execute(AnamorphicFlareFeature& feature, RenderPass& pass);
	};

	struct AnamorphicFlareFeature::ComposeStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AnamorphicFlareFeature::* enableCondition_ = &AnamorphicFlareFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::AnamorphicFlare);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Read(Ping),
			RenderAccess::Write(Radiance)
		};

		static void Execute(AnamorphicFlareFeature& feature, RenderPass& pass);
	};
}
