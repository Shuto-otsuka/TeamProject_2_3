#include <GraphicsEngine/Model/Opaque/GeometryStage.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void GeometryFeature::DepthEarlyStage::Execute(GeometryFeature& feature, RenderPass& pass)
	{
		pass.ClearDepth(Depth);
		feature.depthPipeline_.DrawIndirect(pass, 1, 0, 0, 0, 0);
	}

	void GeometryFeature::EarlyDepthResolveStage::Execute(GeometryFeature& feature, RenderPass& pass)
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
			feature.resolvePipeline_.Dispatch(pass, Max(static_cast<Uint32>(desc.Width) >> mip, 1u), Max(desc.Height >> mip, 1u), 1, mip);
		}
	}

	void GeometryFeature::DepthLateStage::Execute(GeometryFeature& feature, RenderPass& pass)
	{
		feature.depthPipeline_.DrawIndirect(pass, 1, 1, 0, 0, 1);
	}

	void GeometryFeature::DepthResolveStage::Execute(GeometryFeature& feature, RenderPass& pass)
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
			feature.resolvePipeline_.Dispatch(pass, Max(static_cast<Uint32>(desc.Width) >> mip, 1u), Max(desc.Height >> mip, 1u), 1, mip);
		}
	}

	void GeometryFeature::VisibilityStage::Execute(GeometryFeature& feature, RenderPass& pass)
	{
		feature.visibilityPipeline_.DrawIndirect(pass, 1, 0, 0, 0, 0);
		feature.visibilityPipeline_.DrawIndirect(pass, 1, 1, 0, 0, 1);
	}
}
