#include <GraphicsEngine/Model/Transparent/TransparentStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void TransparentFeature::UpdateStage::Execute(TransparentFeature& feature, RenderPass& pass)
	{
		feature.clearPipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}

	void TransparentFeature::DrawStage::Execute(TransparentFeature& feature, RenderPass& pass)
	{
		feature.drawPipeline_.DrawIndirect(pass, 1);
	}

	void TransparentFeature::ResolveStage::Execute(TransparentFeature& feature, RenderPass& pass)
	{
		feature.resolvePipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}
}
