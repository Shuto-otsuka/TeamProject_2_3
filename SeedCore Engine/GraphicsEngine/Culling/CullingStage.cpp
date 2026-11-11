#include <GraphicsEngine/Culling/CullingStage.h>
#include <GraphicsEngine/Model/ModelUnit.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void CullingFeature::CameraEarlyStage::Execute(CullingFeature& feature, RenderPass& pass)
	{
		Uint32 mesh = feature.mesh_ ? 0x80000000u : 0u;
		feature.preparePipeline_.Dispatch(pass, 1, 1, 1, mesh);

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = pass.Resource(DrawArguments);
		pass.List().Get()->ResourceBarrier(1, &barrier);

		feature.earlyPipeline_.Dispatch(pass, ModelUnit::transparentBase_ * 32, 1, 1, mesh | ModelUnit::transparentBase_);
	}

	void CullingFeature::CameraLateStage::Execute(CullingFeature& feature, RenderPass& pass)
	{
		feature.latePipeline_.Dispatch(pass, capacity_, 1, 1, feature.mesh_ ? 0x80000000u : 0u);
	}
}
