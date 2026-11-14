#include <GraphicsEngine/PostProcess/ColorGrading/ColorGradingStage.h>

namespace SeedCore
{
	void ColorGradingFeature::ResolveStage::Execute(ColorGradingFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.resolvePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}
}
