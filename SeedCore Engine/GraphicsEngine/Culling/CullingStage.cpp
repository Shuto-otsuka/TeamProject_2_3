#include <GraphicsEngine/Culling/CullingStage.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void CullingFeature::CullingStage::EarlyStep::Execute(CullingFeature& feature, RenderPass& pass)
	{
		Uint32 mesh = feature.mesh_ ? 0x80000000u : 0u;
		feature.preparePipeline_.Dispatch(pass, 1, 1, 1, mesh);

		StaticArray<D3D12_RESOURCE_BARRIER, 2> barriers{};
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barriers[0].UAV.pResource = pass.Resource(MeshletArguments);
		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barriers[1].UAV.pResource = pass.Resource(ChunkArguments);
		pass.List().Get()->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());

		feature.earlyPipeline_.Dispatch(pass, feature.mesh_ ? GeometryFeature::transparentBase_ : GeometryFeature::transparentBase_ * 32, 1, 1, mesh | GeometryFeature::transparentBase_);
	}

	void CullingFeature::CullingStage::LateStep::Execute(CullingFeature& feature, RenderPass& pass)
	{
		if (feature.mesh_)
		{
			return;
		}
		feature.latePipeline_.Dispatch(pass, capacity_, 1, 1, 0);
	}

	void CullingFeature::CullingStage::TransparentStep::Execute(CullingFeature& feature, RenderPass& pass)
	{
		Uint32 mesh = feature.mesh_ ? 0x80000000u : 0u;
		Uint32 count = GeometryFeature::instanceCapacity_ - GeometryFeature::transparentBase_;
		feature.transparentPipeline_.Dispatch(pass, feature.mesh_ ? count : count * 32, 1, 1, mesh | GeometryFeature::transparentBase_);
	}
}
