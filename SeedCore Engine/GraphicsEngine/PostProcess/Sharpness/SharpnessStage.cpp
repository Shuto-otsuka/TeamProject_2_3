#include <GraphicsEngine/PostProcess/Sharpness/SharpnessStage.h>

namespace SeedCore
{
	void SharpnessFeature::ResolveStage::Execute(SharpnessFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.resolvePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}
}
