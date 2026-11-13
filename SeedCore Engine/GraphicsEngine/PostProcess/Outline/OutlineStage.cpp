#include <GraphicsEngine/PostProcess/Outline/OutlineStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void OutlineFeature::UpdateStage::StyleStep::Execute(OutlineFeature& feature, RenderPass& pass)
	{
		feature.stylePipeline_.Dispatch(pass, static_cast<Uint32>(feature.updates_.size()), 1);
	}

	void OutlineFeature::UpdateStage::MaskStep::Execute(OutlineFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.maskPipeline_.Dispatch(pass, view->renderWidth_, view->renderHeight_);
	}

	void OutlineFeature::ResolveStage::SceneStep::Execute(OutlineFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.resolvePipeline_.Dispatch(pass, view->renderWidth_, view->renderHeight_, 1, view->kind_ == RenderViewKind::Editor ? 1 : 0);
	}

	void OutlineFeature::ResolveStage::CanvasStep::Execute(OutlineFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.canvasPipeline_.Dispatch(pass, view->renderWidth_, view->renderHeight_);
	}

	void OutlineFeature::ComposeStage::Execute(OutlineFeature& feature, RenderPass& pass)
	{
		feature.composePipeline_.Draw(pass);
	}
}
