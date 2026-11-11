#include <GraphicsEngine/PostProcess/Bokeh/BokehStage.h>

namespace SeedCore
{
	void BokehFeature::ResolveStage::Execute(BokehFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.resolvePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}
}
