#include <GraphicsEngine/Debug/DebugViewStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void DebugViewFeature::ResolveStage::Execute(DebugViewFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.resolvePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_, 1, static_cast<Uint32>(view->mode_));
	}
}
