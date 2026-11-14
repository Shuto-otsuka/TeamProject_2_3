#include <GraphicsEngine/Shape/Primitive/PrimitiveStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void PrimitiveFeature::UpdateStage::Execute(PrimitiveFeature& feature, RenderPass& pass)
	{
		Uint32 solidCount = static_cast<Uint32>(feature.solid_.Updates().size());
		if (solidCount > 0)
		{
			feature.updatePipeline_.Dispatch(pass, solidCount, 1, 1, 0);
		}
		Uint32 materialCount = static_cast<Uint32>(feature.solid_.Materials().size());
		if (materialCount > 0)
		{
			feature.updatePipeline_.Dispatch(pass, materialCount, 1, 1, 1);
		}
	}

	void PrimitiveFeature::SubmitStage::ExpandStep::Execute(PrimitiveFeature& feature, RenderPass& pass)
	{
		feature.expandPipeline_.Dispatch(pass, feature.solid_.Count(), 1, 1);
	}

	void PrimitiveFeature::SubmitStage::RaytracingStep::Execute(PrimitiveFeature& feature, RenderPass& pass)
	{
		feature.raytracingPipeline_.Dispatch(pass, feature.solid_.Count(), 1, 1, feature.solid_.Count());
	}

	void PrimitiveFeature::DrawStage::WireframeStep::Execute(PrimitiveFeature& feature, RenderPass& pass)
	{
		Uint32 count = static_cast<Uint32>(feature.wireframe_.Instances(pass.View()->kind_).size());
		feature.wireframePipeline_.Draw(pass, count * 3);
	}

	void PrimitiveFeature::DrawStage::CanvasStep::Execute(PrimitiveFeature& feature, RenderPass& pass)
	{
		Uint32 count = static_cast<Uint32>(feature.wireframe_.Instances(pass.View()->kind_).size());
		feature.canvasPipeline_.Draw(pass, count * 3);
	}
}
