#include <GraphicsEngine/Model/Material/MaterialStage.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void MaterialFeature::ClassifyStage::Execute(MaterialFeature& feature, RenderPass& pass)
	{
		feature.preparePipeline_.Dispatch(pass, 1025, 1, 1);

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = pass.Resource(Buckets);
		pass.List().Get()->ResourceBarrier(1, &barrier);

		feature.classifyPipeline_.Dispatch(pass, pass.Width(), pass.Height(), 1);
	}

	void MaterialFeature::PrefixSumStage::Execute(MaterialFeature& feature, RenderPass& pass)
	{
		feature.prefixSumPipeline_.Dispatch(pass, 1024, 1, 1);
	}

	void MaterialFeature::ScatterStage::Execute(MaterialFeature& feature, RenderPass& pass)
	{
		feature.scatterPipeline_.Dispatch(pass, pass.Width(), pass.Height(), 1);
	}

	void MaterialFeature::ResolveStage::Execute(MaterialFeature& feature, RenderPass& pass)
	{
		Uint32 pixelCount = pass.Width() * pass.Height();
		feature.resolvePipeline_.Dispatch(pass, 65536, (pixelCount + 65535) / 65536, 1);
	}
}
