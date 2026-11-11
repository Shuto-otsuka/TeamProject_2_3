#include <GraphicsEngine/PostProcess/ToneMapping/ToneMappingStage.h>

namespace SeedCore
{
	void ToneMappingFeature::ResolveStage::Execute(ToneMappingFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.resolvePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}
}
