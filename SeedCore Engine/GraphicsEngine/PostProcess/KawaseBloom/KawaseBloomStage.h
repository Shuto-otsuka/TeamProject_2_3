#pragma once
#include <GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomFeature.h>
#include <GraphicsEngine/PostProcess/Bokeh/BokehFeature.h>

namespace SeedCore
{
	struct KawaseBloomFeature::FilterStage
	{
		struct PrefilterStep;

		struct DownsampleStep;

		struct UpsampleStep;

		using Sequence = RenderSequence<PrefilterStep, DownsampleStep, UpsampleStep>;
	};

	struct KawaseBloomFeature::FilterStage::PrefilterStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::Bloom);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(BokehFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Radiance)
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};

	struct KawaseBloomFeature::FilterStage::DownsampleStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::Bloom);

		SC_CONSTEX Uint32 repeat_ = 5;

		SC_CONSTEX RenderAccess accesses_[][2] =
		{
			{
				RenderAccess::Read(Radiance).As("Source"),
				RenderAccess::Write(Level1).As("Destination")
			},
			{
				RenderAccess::Read(Level1).As("Source"),
				RenderAccess::Write(Level2).As("Destination")
			},
			{
				RenderAccess::Read(Level2).As("Source"),
				RenderAccess::Write(Level3).As("Destination")
			},
			{
				RenderAccess::Read(Level3).As("Source"),
				RenderAccess::Write(Level4).As("Destination")
			},
			{
				RenderAccess::Read(Level4).As("Source"),
				RenderAccess::Write(Level5).As("Destination")
			}
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};

	struct KawaseBloomFeature::FilterStage::UpsampleStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool KawaseBloomFeature::* enableCondition_ = &KawaseBloomFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::Bloom);

		SC_CONSTEX Uint32 repeat_ = 5;

		SC_CONSTEX RenderAccess accesses_[][3] =
		{
			{
				RenderAccess::Constant(Constants),
				RenderAccess::Read(Level5).As("Source"),
				RenderAccess::Write(Level4).As("Destination")
			},
			{
				RenderAccess::Constant(Constants),
				RenderAccess::Read(Level4).As("Source"),
				RenderAccess::Write(Level3).As("Destination")
			},
			{
				RenderAccess::Constant(Constants),
				RenderAccess::Read(Level3).As("Source"),
				RenderAccess::Write(Level2).As("Destination")
			},
			{
				RenderAccess::Constant(Constants),
				RenderAccess::Read(Level2).As("Source"),
				RenderAccess::Write(Level1).As("Destination")
			},
			{
				RenderAccess::Constant(Constants),
				RenderAccess::Read(Level1).As("Source"),
				RenderAccess::Write(Radiance).As("Destination")
			}
		};

		static void Execute(KawaseBloomFeature& feature, RenderPass& pass);
	};
}
