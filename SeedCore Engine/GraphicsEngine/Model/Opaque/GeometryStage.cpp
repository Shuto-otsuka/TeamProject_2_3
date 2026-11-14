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
		if (feature.mesh_ && iteration == 1)
		{
			feature.depthPipeline_.DrawGroups(pass, 1024, 2, 2);
		}
	}

	void GeometryFeature::DrawStage::PyramidStep::Execute(GeometryFeature& feature, RenderPass& pass)
	{
		ID3D12Resource* hiZ = pass.Resource(HiZ);
		D3D12_RESOURCE_DESC desc = hiZ->GetDesc();
		for (Uint32 mip = 0;mip < desc.MipLevels;++mip)
		{
			if (mip > 0)
			{
				D3D12_RESOURCE_BARRIER barrier{};
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				barrier.UAV.pResource = hiZ;
				pass.List().Get()->ResourceBarrier(1, &barrier);
			}
			feature.pyramidPipeline_.Dispatch(pass, Max(static_cast<Uint32>(desc.Width) >> mip, 1u), Max(desc.Height >> mip, 1u), 1, mip);
		}
	}

	void GeometryFeature::DrawStage::VisibilityStep::Execute(GeometryFeature& feature, RenderPass& pass)
	{
		Uint32 replay = feature.mesh_ ? 0x100u : 0u;
		pass.ClearTarget(VisibilityID);
		feature.visibilityPipeline_.DrawIndirect(pass, 1, 0, 0, 0, replay);
		feature.visibilityPipeline_.DrawIndirect(pass, 1, 1, 0, 0, replay | 1);
		if (feature.mesh_)
		{
			feature.visibilityPipeline_.DrawGroups(pass, 1024, 2, replay | 2);
		}
	}
}
