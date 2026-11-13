#include <GraphicsEngine/Model/ModelStage.h>

namespace SeedCore
{
	void ModelFeature::UpdateStage::Execute(ModelFeature& feature, RenderPass& pass)
	{
		StaticArray<Size, 6> counts =
		{
			feature.units_.Updates().size(),
			feature.meshes_.MeshUpdates().size(),
			feature.meshes_.ClusterUpdates().size(),
			feature.meshes_.PlacementUpdates().size(),
			feature.materials_.Updates().size(),
			feature.raytracing_.Updates().size(),
		};
		for (Uint32 table = 0;table < static_cast<Uint32>(counts.size());++table)
		{
			if (counts[table] == 0)
			{
				continue;
			}
			feature.updatePipeline_.Dispatch(pass, static_cast<Uint32>(counts[table]), 1, 1, table);
		}
	}

	void ModelFeature::DeformStage::Execute(ModelFeature& feature, RenderPass& pass)
	{
		feature.deformPipeline_.Dispatch(pass, feature.raytracing_.DeformVertexCount(), static_cast<Uint32>(feature.raytracing_.Deforms().size()));
	}

	void ModelFeature::SubmitStage::ExpandStep::Execute(ModelFeature& feature, RenderPass& pass)
	{
		feature.clearPipeline_.Dispatch(pass, Max(feature.meshes_.Count() * 2, 1u), 1, 1);

		Uint32 softbodyCount = static_cast<Uint32>(feature.softbodies_.Instances().size());
		feature.preparePipeline_.Dispatch(pass, Max(softbodyCount, 1u), 1, 1);

		StaticArray<D3D12_RESOURCE_BARRIER, 3> barriers{};
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barriers[0].UAV.pResource = pass.Resource(GeometryFeature::Instances);
		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barriers[1].UAV.pResource = pass.Resource(GeometryFeature::Counts);
		barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barriers[2].UAV.pResource = pass.Resource(Streaming);
		pass.List().Get()->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());

		feature.expandPipeline_.Dispatch(pass, feature.units_.Count(), 1, 1);
	}

	void ModelFeature::SubmitStage::ReadbackStep::Execute(ModelFeature& feature, RenderPass& pass)
	{
		Uint32 slot = FrameRing::Index();
		feature.readbackCounts_[slot] = feature.meshes_.Count();
		if (feature.readbackCounts_[slot] == 0)
		{
			return;
		}
		pass.List().Get()->CopyBufferRegion(feature.readbacks_[slot]->Resource(), 0, pass.Resource(Streaming), 0, static_cast<Uint64>(feature.readbackCounts_[slot]) * 2 * sizeof(Uint32));
	}

	void ModelFeature::SubmitStage::RaytracingStep::Execute(ModelFeature& feature, RenderPass& pass)
	{
		Uint32 count = feature.raytracing_.Count();
		feature.raytracingPipeline_.Dispatch(pass, count, 1, 1, count);
	}
}
