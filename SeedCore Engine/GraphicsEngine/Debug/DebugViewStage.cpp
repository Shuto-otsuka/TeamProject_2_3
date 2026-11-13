#include <GraphicsEngine/Debug/DebugViewStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void DebugViewFeature::UpdateStage::Execute(DebugViewFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.clearPipeline_.Dispatch(pass, view->renderWidth_, view->renderHeight_);
	}

	void DebugViewFeature::DrawStage::WireframeStep::Execute(DebugViewFeature& feature, RenderPass& pass)
	{
		pass.ClearTarget(HUDFeature::DisplayColor, { 0.2f, 0.2f, 0.2f, 1.0f });
		feature.wireframePipeline_.DrawIndirect(pass, 1, 0, 0, 0, 0);
		feature.wireframePipeline_.DrawIndirect(pass, 1, 1, 0, 0, 1);
	}

	void DebugViewFeature::DrawStage::OverdrawStep::Execute(DebugViewFeature& feature, RenderPass& pass)
	{
		feature.overdrawPipeline_.DrawIndirect(pass, 1, 0, 0, 0, 0);
		feature.overdrawPipeline_.DrawIndirect(pass, 1, 1, 0, 0, 1);
	}

	void DebugViewFeature::ResolveStage::Execute(DebugViewFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.resolvePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_, 1, static_cast<Uint32>(view->mode_));
	}

	void DebugViewFeature::ComposeStage::Execute(DebugViewFeature& feature, RenderPass& pass)
	{
		feature.boundsPipeline_.Dispatch(pass, GeometryFeature::transparentBase_ * 32, 1, 1);
	}
}
