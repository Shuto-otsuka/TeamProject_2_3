#pragma once
#include <GraphicsEngine/PostProcess/AutoExposure/AutoExposureFeature.h>
#include <GraphicsEngine/PostProcess/Upscale/UpscaleFeature.h>

namespace SeedCore
{
	struct AutoExposureFeature::BuildStage
	{
		struct HistogramStep;

		struct AverageStep;

		using Sequence = RenderSequence<HistogramStep, AverageStep>;
	};

	struct AutoExposureFeature::BuildStage::HistogramStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AutoExposureFeature::* enableCondition_ = &AutoExposureFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::AutoExposure);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(UpscaleFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Histogram)
		};

		static void Execute(AutoExposureFeature& feature, RenderPass& pass);
	};

	struct AutoExposureFeature::BuildStage::AverageStep :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AutoExposureFeature::* enableCondition_ = &AutoExposureFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::AutoExposure);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Histogram),
			RenderAccess::Write(Exposure)
		};

		static void Execute(AutoExposureFeature& feature, RenderPass& pass);
	};
}
