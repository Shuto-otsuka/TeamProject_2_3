#include <GraphicsEngine/PostProcess/Vignette/VignetteStage.h>

namespace SeedCore
{
	void VignetteFeature::ResolveStage::Execute(VignetteFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.resolvePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}
}
