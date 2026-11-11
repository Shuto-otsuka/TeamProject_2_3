#include <GraphicsEngine/PostProcess/AutoExposure/AutoExposureStage.h>

namespace SeedCore
{
	void AutoExposureFeature::HistogramStage::Execute(AutoExposureFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.histogramPipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}

	void AutoExposureFeature::AverageStage::Execute(AutoExposureFeature& feature, RenderPass& pass)
	{
		feature.averagePipeline_.Dispatch(pass, 1, 1);
	}
}
