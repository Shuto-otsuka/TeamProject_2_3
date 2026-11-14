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
		Uint32 replay = feature.mesh_ ? 0x100u : 0u;
		pass.ClearTarget(HUDFeature::Display, { 0.3f, 0.3f, 0.3f, 1.0f });
		feature.wireframePipeline_.DrawIndirect(pass, 1, 0, 0, 0, replay);
		feature.wireframePipeline_.DrawIndirect(pass, 1, 1, 0, 0, replay | 1);
		if (feature.mesh_)
		{
			feature.wireframePipeline_.DrawGroups(pass, 1024, 2, replay | 2);
		}
	}

	void DebugViewFeature::DrawStage::OverdrawStep::Execute(DebugViewFeature& feature, RenderPass& pass)
	{
		Uint32 replay = feature.mesh_ ? 0x100u : 0u;
		feature.overdrawPipeline_.DrawIndirect(pass, 1, 0, 0, 0, replay);
		feature.overdrawPipeline_.DrawIndirect(pass, 1, 1, 0, 0, replay | 1);
		if (feature.mesh_)
		{
			feature.overdrawPipeline_.DrawGroups(pass, 1024, 2, replay | 2);
		}
	}

	void DebugViewFeature::ResolveStage::Execute(DebugViewFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.resolvePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_, 1, static_cast<Uint32>(view->mode_));
	}

	void DebugViewFeature::ComposeStage::Execute(DebugViewFeature& feature, RenderPass& pass)
	{
		feature.boundsPipeline_.Dispatch(pass, feature.mesh_ ? 65536 : GeometryFeature::transparentBase_ * 32, 1, 1, feature.mesh_ ? 1 : 0);
	}
}
