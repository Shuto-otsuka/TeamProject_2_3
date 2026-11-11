#pragma once
#include <GraphicsEngine/PostProcess/AutoExposure/AutoExposureFeature.h>
#include <GraphicsEngine/PostProcess/Upscale/UpscaleFeature.h>

namespace SeedCore
{
	struct AutoExposureFeature::HistogramStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AutoExposureFeature::* enableCondition_ = &AutoExposureFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(UpscaleFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Histogram)
		};

		static void Execute(AutoExposureFeature& feature, RenderPass& pass);
	};

	struct AutoExposureFeature::AverageStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool AutoExposureFeature::* enableCondition_ = &AutoExposureFeature::enabled_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Histogram),
			RenderAccess::Write(Exposure)
		};

		static void Execute(AutoExposureFeature& feature, RenderPass& pass);
	};
}
