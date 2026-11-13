#include <GraphicsEngine/Model/Opaque/GeometryStage.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void GeometryFeature::UpdateStage::Execute(GeometryFeature& feature, RenderPass& pass)
	{
		feature.updatePipeline_.Dispatch(pass, 4, 1);
	}

	void GeometryFeature::DrawStage::DepthStep::Execute(GeometryFeature& feature, RenderPass& pass)
	{
		Uint32 iteration = pass.Iteration();
		if (iteration == 0)
		{
			pass.ClearDepth(Depth);
		}
		feature.depthPipeline_.DrawIndirect(pass, 1, iteration, 0, 0, iteration);
	}

	void GeometryFeature::DrawStage::PyramidStep::Execute(GeometryFeature& feature, RenderPass& pass)
	{
		ID3D12Resource* hierarchicalDepth = pass.Resource(HierarchicalDepth);
		D3D12_RESOURCE_DESC desc = hierarchicalDepth->GetDesc();
		for (Uint32 mip = 0;mip < desc.MipLevels;++mip)
		{
			if (mip > 0)
			{
				D3D12_RESOURCE_BARRIER barrier{};
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				barrier.UAV.pResource = hierarchicalDepth;
				pass.List().Get()->ResourceBarrier(1, &barrier);
			}
			feature.pyramidPipeline_.Dispatch(pass, Max(static_cast<Uint32>(desc.Width) >> mip, 1u), Max(desc.Height >> mip, 1u), 1, mip);
		}
	}

	void GeometryFeature::DrawStage::VisibilityStep::Execute(GeometryFeature& feature, RenderPass& pass)
	{
		pass.ClearTarget(VisibilityId);
		feature.visibilityPipeline_.DrawIndirect(pass, 1, 0, 0, 0, 0);
		feature.visibilityPipeline_.DrawIndirect(pass, 1, 1, 0, 0, 1);
	}
}
