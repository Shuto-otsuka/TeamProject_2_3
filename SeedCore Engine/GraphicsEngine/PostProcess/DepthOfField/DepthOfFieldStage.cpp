#include <GraphicsEngine/PostProcess/DepthOfField/DepthOfFieldStage.h>

namespace SeedCore
{
	void DepthOfFieldFeature::FilterStage::Execute(DepthOfFieldFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.blurPipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}
}
