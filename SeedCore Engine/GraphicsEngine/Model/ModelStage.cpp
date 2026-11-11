#include <GraphicsEngine/Model/ModelStage.h>

namespace SeedCore
{
	void ModelFeature::UpdateStage::Execute(ModelFeature& feature, RenderPass& pass)
	{
		StaticArray<Size, 5> counts =
		{
			feature.units_.Updates().size(),
			feature.meshes_.MeshUpdates().size(),
			feature.meshes_.ClusterUpdates().size(),
			feature.meshes_.PlacementUpdates().size(),
			feature.materials_.Updates().size(),
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

	void ModelFeature::ExpandStage::Execute(ModelFeature& feature, RenderPass& pass)
	{
		Uint32 softbodyCount = static_cast<Uint32>(feature.softbodies_.Instances().size());
		feature.preparePipeline_.Dispatch(pass, Max(softbodyCount, 1u), 1, 1);

		StaticArray<D3D12_RESOURCE_BARRIER, 2> barriers{};
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barriers[0].UAV.pResource = pass.Resource(Instances);
		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barriers[1].UAV.pResource = pass.Resource(Counts);
		pass.List().Get()->ResourceBarrier(static_cast<Uint32>(barriers.size()), barriers.data());

		feature.expandPipeline_.Dispatch(pass, feature.units_.Count(), 1, 1);
	}
}
