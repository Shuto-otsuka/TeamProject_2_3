#include <GraphicsEngine/PostProcess/PostProcessStage.h>

namespace SeedCore
{
	void PostProcessFeature::ComposeStage::Execute(PostProcessFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.composePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}
}
