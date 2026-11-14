#include <GraphicsEngine/Raytracing/BottomLevelStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>

namespace SeedCore
{
	void BottomLevelFeature::UpdateStage::Execute(BottomLevelFeature& feature, RenderPass& pass)
	{
		if (feature.clearing_)
		{
			feature.clearPipeline_.Dispatch(pass, tableCapacity_, 1, 1, tableCapacity_);

			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier.UAV.pResource = pass.Resource(Table);
			pass.List().Get()->ResourceBarrier(1, &barrier);
			feature.clearing_ = false;
		}

		Uint32 count = static_cast<Uint32>(feature.updates_.size());
		if (count > 0)
		{
			feature.updatePipeline_.Dispatch(pass, count, 1, 1, count);
		}
	}

	void BottomLevelFeature::BuildStage::StaticStep::Execute(BottomLevelFeature& feature, RenderPass& pass)
	{
		ID3D12GraphicsCommandList6* list = pass.List().Get();

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = nullptr;
		list->ResourceBarrier(1, &barrier);

		for (const Build& build : feature.staticBuilds_)
		{
			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
			buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			buildDesc.Inputs.Flags = build.flags_;
			buildDesc.Inputs.NumDescs = 1;
			buildDesc.Inputs.pGeometryDescs = &build.geometry_;
			buildDesc.DestAccelerationStructureData = build.resource_->GetGPUVirtualAddress();
			buildDesc.ScratchAccelerationStructureData = feature.scratch_->GetGPUVirtualAddress() + build.scratchOffset_;
			list->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
		}

		list->ResourceBarrier(1, &barrier);
	}

	void BottomLevelFeature::BuildStage::DynamicStep::Execute(BottomLevelFeature& feature, RenderPass& pass)
	{
		ID3D12GraphicsCommandList6* list = pass.List().Get();
		D3D12_GPU_VIRTUAL_ADDRESS vertices = pass.Resource(Vertices)->GetGPUVirtualAddress();

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrier.UAV.pResource = nullptr;
		list->ResourceBarrier(1, &barrier);

		for (const Build& build : feature.dynamicBuilds_)
		{
			D3D12_RAYTRACING_GEOMETRY_DESC geometry = build.geometry_;
			geometry.Triangles.VertexBuffer.StartAddress = vertices + static_cast<Uint64>(build.vertexOffset_) * sizeof(Vector3);
			geometry.Triangles.VertexBuffer.StrideInBytes = sizeof(Vector3);

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
			buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			buildDesc.Inputs.Flags = build.flags_;
			buildDesc.Inputs.NumDescs = 1;
			buildDesc.Inputs.pGeometryDescs = &geometry;
			buildDesc.DestAccelerationStructureData = build.resource_->GetGPUVirtualAddress();
			buildDesc.ScratchAccelerationStructureData = feature.scratch_->GetGPUVirtualAddress() + build.scratchOffset_;
			if ((build.flags_ & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE) != 0)
			{
				buildDesc.SourceAccelerationStructureData = buildDesc.DestAccelerationStructureData;
			}
			list->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
		}

		list->ResourceBarrier(1, &barrier);
	}
}
