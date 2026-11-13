#include <GraphicsEngine/Raytracing/TopLevelStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>

namespace SeedCore
{
	void TopLevelFeature::UpdateStage::Execute(TopLevelFeature& feature, RenderPass& pass)
	{
		feature.updatePipeline_.Dispatch(pass, 1, 1);
	}

	void TopLevelFeature::BuildStage::ResolveStep::Execute(TopLevelFeature& feature, RenderPass& pass)
	{
		if (feature.count_ == 0)
		{
			return;
		}
		feature.resolvePipeline_.Dispatch(pass, feature.count_, 1, 1, feature.count_);
	}

	void TopLevelFeature::BuildStage::StructureStep::Execute(TopLevelFeature& feature, RenderPass& pass)
	{
		ID3D12GraphicsCommandList6* list = pass.List().Get();

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = feature.scratch_.Get();
		list->ResourceBarrier(1, &barrier);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
		buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		buildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
		buildDesc.Inputs.NumDescs = feature.count_;
		buildDesc.Inputs.InstanceDescs = pass.Resource(Descs)->GetGPUVirtualAddress();
		buildDesc.DestAccelerationStructureData = pass.Resource(Structure)->GetGPUVirtualAddress();
		buildDesc.ScratchAccelerationStructureData = feature.scratch_->GetGPUVirtualAddress();
		list->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
	}
}
