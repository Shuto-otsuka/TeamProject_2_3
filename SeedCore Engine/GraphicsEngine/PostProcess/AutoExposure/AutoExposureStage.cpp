#include <GraphicsEngine/PostProcess/AutoExposure/AutoExposureStage.h>

namespace SeedCore
{
	void AutoExposureFeature::BuildStage::HistogramStep::Execute(AutoExposureFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.histogramPipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}

	void AutoExposureFeature::BuildStage::AverageStep::Execute(AutoExposureFeature& feature, RenderPass& pass)
	{
		feature.averagePipeline_.Dispatch(pass, 1, 1);
	}
}
