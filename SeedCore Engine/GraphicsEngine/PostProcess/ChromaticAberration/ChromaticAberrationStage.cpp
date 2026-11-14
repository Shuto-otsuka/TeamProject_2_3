#include <GraphicsEngine/PostProcess/ChromaticAberration/ChromaticAberrationStage.h>

namespace SeedCore
{
	void ChromaticAberrationFeature::ResolveStage::Execute(ChromaticAberrationFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.resolvePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}
}
