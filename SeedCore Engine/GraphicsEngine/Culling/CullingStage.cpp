#include <GraphicsEngine/Culling/CullingStage.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void CullingFeature::CullingStage::EarlyStep::Execute(CullingFeature& feature, RenderPass& pass)
	{
		Uint32 mesh = feature.mesh_ ? 0x80000000u : 0u;
		feature.preparePipeline_.Dispatch(pass, 1, 1, 1, mesh);

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = pass.Resource(DrawArguments);
		pass.List().Get()->ResourceBarrier(1, &barrier);

		feature.earlyPipeline_.Dispatch(pass, GeometryFeature::transparentBase_ * 32, 1, 1, mesh | GeometryFeature::transparentBase_);
	}

	void CullingFeature::CullingStage::LateStep::Execute(CullingFeature& feature, RenderPass& pass)
	{
		feature.latePipeline_.Dispatch(pass, capacity_, 1, 1, feature.mesh_ ? 0x80000000u : 0u);
	}

	void CullingFeature::CullingStage::TransparentStep::Execute(CullingFeature& feature, RenderPass& pass)
	{
		Uint32 mesh = feature.mesh_ ? 0x80000000u : 0u;
		feature.transparentPipeline_.Dispatch(pass, (GeometryFeature::instanceCapacity_ - GeometryFeature::transparentBase_) * 32, 1, 1, mesh | GeometryFeature::transparentBase_);
	}
}
