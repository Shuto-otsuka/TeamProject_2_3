#include <GraphicsEngine/Preview/PreviewStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void PreviewFeature::DrawStage::MeshStep::Execute(PreviewFeature& feature, RenderPass& pass)
	{
		pass.ClearDepth(Depth);
		pass.ClearTarget(Output);
		Uint32 count = static_cast<Uint32>(feature.meshlets_.size() / 2);
		if (count > 0)
		{
			feature.meshPipeline_.Draw(pass, count);
		}
	}

	void PreviewFeature::DrawStage::LineStep::Execute(PreviewFeature& feature, RenderPass& pass)
	{
		feature.linePipeline_.Draw(pass, static_cast<Uint32>(feature.lines_.size()));
	}
}
