#include <GraphicsEngine/Model/ModelMesh.h>
#include <GraphicsEngine/Model/Crister/Crister.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	ModelMesh::ModelMesh()
	{
		/// No Code
	}

	ModelMesh::~ModelMesh() = default;

	void ModelMesh::Begin(Uint64 frame)
	{
		frame_ = frame;
		meshUpdates_.clear();
		clusterUpdates_.clear();
		placementUpdates_.clear();
	}

	const ModelMeshRecord* ModelMesh::Register(Crister& crister)
	{
		auto found = records_.find(&crister);
		if (found != records_.end())
		{
			found->second.usedFrame_ = frame_;
			return &found->second;
		}

		const DynamicArray<SubMesh>& subMeshes = crister.SubMeshes();
		Uint32 placementTotal = 0;
		for (Size subMeshIndex = 0;subMeshIndex < subMeshes.size();++subMeshIndex)
		{
			placementTotal += static_cast<Uint32>(crister.Residency().SubMeshPlacement(subMeshIndex).size());
		}
		Uint32 clusterTotal = static_cast<Uint32>(crister.Clusters().size());
		if (meshCount_ + subMeshes.size() > meshCapacity_ || clusterCount_ + clusterTotal > clusterCapacity_ || placementCount_ + placementTotal > placementCapacity_)
		{
			if (!capacityLogged_)
			{
				SC_LOG_ERROR("ModelMesh: 常駐の表の上限（メッシュ {}、クラスタ {}、配置 {}）を超えたので、このモデルは描きません", meshCapacity_, clusterCapacity_, placementCapacity_);
				capacityLogged_ = true;
			}
			return nullptr;
		}

		ModelMeshRecord& record = records_[&crister];
		record.meshOffset_ = meshCount_;
		record.meshCount_ = static_cast<Uint32>(subMeshes.size());
		record.clusterOffset_ = clusterCount_;
		record.surfaceCount_ = Max(static_cast<Uint32>(crister.Surfaces().size()), 1u);
		record.clusterFlags_.assign(clusterTotal, SC_INVALID);
		record.morphNodes_.assign(subMeshes.size(), -1);
		record.morphWeightOffsets_.assign(subMeshes.size(), 0);
		record.usedFrame_ = frame_;
		meshCount_ += record.meshCount_;
		clusterCount_ += clusterTotal;

		const DynamicArray<Skin>& skins = crister.Skins();
		for (const Skin& skin : skins)
		{
			record.boneCount_ += static_cast<Uint32>(skin.joints_.size());
		}

		const DynamicArray<Node>& nodes = crister.Nodes();
		for (Size subMeshIndex = 0;subMeshIndex < subMeshes.size();++subMeshIndex)
		{
			const SubMesh& subMesh = subMeshes[subMeshIndex];
			std::span<const Matrix> placements = crister.Residency().SubMeshPlacement(subMeshIndex);

			ModelMeshUpdate update;
			update.index_ = record.meshOffset_ + static_cast<Uint32>(subMeshIndex);
			ModelMeshStructuredBuffer& mesh = update.data_;
			mesh.positionMin_ = crister.PositionMin();
			mesh.positionExtent_ = crister.PositionExtent();
			mesh.texcoordMinU_ = crister.TexcoordMin().x;
			mesh.texcoordMinV_ = crister.TexcoordMin().y;
			mesh.texcoordExtent_ = crister.TexcoordExtent();
			mesh.texcoordDensity_ = subMesh.texcoordDensity_;
			mesh.surfaceIndex_ = subMesh.surfaceIndex_ < record.surfaceCount_ ? subMesh.surfaceIndex_ : 0;
			mesh.clusterOffset_ = record.clusterOffset_ + subMesh.clusterOffset_;
			mesh.clusterCount_ = subMesh.clusterCount_;
			mesh.placementOffset_ = placementCount_;
			mesh.placementCount_ = static_cast<Uint32>(placements.size());
			mesh.skinVertexBufferIndex_ = crister.Residency().SkinVertexBufferIndex();
			mesh.morphDeltaBufferIndex_ = crister.Residency().MorphDeltaBufferIndex();
			mesh.vertexMorphSourceBufferIndex_ = crister.Residency().VertexMorphSourceBufferIndex();
			mesh.morphDeltaOffset_ = subMesh.morphDeltaOffset_;
			mesh.vertexOffset_ = subMesh.vertexOffset_;
			mesh.vertexCount_ = subMesh.vertexCount_;

			if (subMesh.skinIndex_ >= 0 && subMesh.skinIndex_ < static_cast<Int>(skins.size()))
			{
				mesh.skinIndex_ = static_cast<Uint>(subMesh.skinIndex_);
				for (Int skinIndex = 0;skinIndex < subMesh.skinIndex_;++skinIndex)
				{
					mesh.skinBoneOffset_ += static_cast<Uint>(skins[skinIndex].joints_.size());
				}
			}

			if (!subMesh.morphs_.empty())
			{
				for (Size nodeIndex = 0;nodeIndex < nodes.size();++nodeIndex)
				{
					if (nodes[nodeIndex].mesh_ == subMesh.meshIndex_)
					{
						record.morphNodes_[subMeshIndex] = static_cast<Int>(nodeIndex);
						break;
					}
				}
				record.morphWeightOffsets_[subMeshIndex] = record.morphWeightCount_;
				mesh.morphTargetCount_ = static_cast<Uint>(subMesh.morphs_.size());
				mesh.morphWeightOffset_ = record.morphWeightCount_;
				record.morphWeightCount_ += static_cast<Uint32>(subMesh.morphs_.size());
			}
			meshUpdates_.push_back(update);

			for (const Matrix& placement : placements)
			{
				ModelPlacementUpdate placementUpdate;
				placementUpdate.index_ = placementCount_;
				placementUpdate.data_ = placement;
				placementUpdates_.push_back(placementUpdate);
				++placementCount_;
			}
		}
		return &record;
	}

	void ModelMesh::End(BindlessHeap& heap)
	{
		for (auto& entry : records_)
		{
			Crister& crister = *entry.first;
			ModelMeshRecord& record = entry.second;
			if (record.usedFrame_ != frame_)
			{
				continue;
			}

			const DynamicArray<Cluster>& clusters = crister.Clusters();
			for (Uint32 clusterIndex = 0;clusterIndex < static_cast<Uint32>(clusters.size());++clusterIndex)
			{
				Bool resident = crister.Residency().ClusterResident(clusterIndex);
				Uint32 flags = resident ? 1u : 0u;
				if (resident)
				{
					flags |= crister.Residency().StandaloneVertices(clusterIndex) ? 2u : 0u;
					crister.Residency().TouchCluster(clusterIndex, frame_);
				}
				if (flags == record.clusterFlags_[clusterIndex])
				{
					continue;
				}
				record.clusterFlags_[clusterIndex] = flags;

				ModelClusterUpdate update;
				update.index_ = record.clusterOffset_ + clusterIndex;
				ModelClusterStructuredBuffer& cluster = update.data_;
				cluster.meshletOffset_ = clusters[clusterIndex].meshletOffset_;
				cluster.meshletCount_ = clusters[clusterIndex].meshletCount_;
				cluster.lodError_ = clusters[clusterIndex].lodError_;
				cluster.flags_ = flags;
				if (resident)
				{
					cluster.vertexBufferIndex_ = crister.Residency().ClusterVertexBufferIndex(clusterIndex);
					cluster.meshletBufferIndex_ = crister.Residency().ClusterMeshletBufferIndex(clusterIndex);
					cluster.meshletBoundBufferIndex_ = crister.Residency().ClusterMeshletBoundBufferIndex(clusterIndex);
					cluster.vertexIndicesBufferIndex_ = crister.Residency().ClusterVertexIndicesBufferIndex(clusterIndex);
					cluster.primitiveIndicesBufferIndex_ = crister.Residency().ClusterPrimitiveIndicesBufferIndex(clusterIndex);
				}
				clusterUpdates_.push_back(update);
			}
		}

		CristerResidency::EvictClusterBudget(&heap, frame_);
		CristerResidency::EvictTextureBudget(&heap, frame_);
	}

	std::span<const ModelMeshUpdate> ModelMesh::MeshUpdates()const
	{
		return meshUpdates_;
	}

	std::span<const ModelClusterUpdate> ModelMesh::ClusterUpdates()const
	{
		return clusterUpdates_;
	}

	std::span<const ModelPlacementUpdate> ModelMesh::PlacementUpdates()const
	{
		return placementUpdates_;
	}
}
