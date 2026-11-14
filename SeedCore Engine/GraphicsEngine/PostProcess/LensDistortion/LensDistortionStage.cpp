#include <GraphicsEngine/PostProcess/LensDistortion/LensDistortionStage.h>

namespace SeedCore
{
	void LensDistortionFeature::ResolveStage::Execute(LensDistortionFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.resolvePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}
}
