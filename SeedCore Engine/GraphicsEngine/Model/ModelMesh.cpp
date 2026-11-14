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
		used_.clear();
	}

	const ModelMeshRecord* ModelMesh::Register(const Handle<Crister>& handle, Crister& crister)
	{
		Uint32 recordIndex = static_cast<Uint32>(handle.index_);
		if (records_.size() <= recordIndex)
		{
			records_.resize(static_cast<Size>(recordIndex) + 1);
		}
		ModelMeshRecord& record = records_[recordIndex];
		if (record.registered_ && record.generation_ == handle.generation_)
		{
			if (record.usedFrame_ != frame_)
			{
				record.usedFrame_ = frame_;
				record.crister_ = &crister;
				used_.push_back(recordIndex);
			}
			return &record;
		}
		if (record.registered_)
		{
			Release(record);
		}

		const std::span<const SubMesh> subMeshes = crister.SubMeshes();
		Uint32 placementTotal = 0;
		for (Size subMeshIndex = 0;subMeshIndex < subMeshes.size();++subMeshIndex)
		{
			placementTotal += static_cast<Uint32>(crister.Residency().SubMeshPlacement(subMeshIndex).size());
		}
		Uint32 meshTotal = static_cast<Uint32>(subMeshes.size());
		Uint32 clusterTotal = static_cast<Uint32>(crister.Clusters().size());
		Uint32 meshOffset = Allocate(freeMeshes_, meshCount_, meshTotal, meshCapacity_);
		Uint32 clusterOffset = Allocate(freeClusters_, clusterCount_, clusterTotal, clusterCapacity_);
		Uint32 placementOffset = Allocate(freePlacements_, placementCount_, placementTotal, placementCapacity_);
		if (meshOffset == SC_INVALID || clusterOffset == SC_INVALID || placementOffset == SC_INVALID)
		{
			if (meshOffset != SC_INVALID)
			{
				Free(freeMeshes_, meshCount_, meshOffset, meshTotal);
			}
			if (clusterOffset != SC_INVALID)
			{
				Free(freeClusters_, clusterCount_, clusterOffset, clusterTotal);
			}
			if (placementOffset != SC_INVALID)
			{
				Free(freePlacements_, placementCount_, placementOffset, placementTotal);
			}
			if (!capacityLogged_)
			{
				SC_LOG_ERROR("ModelMesh: 常駐の表の上限（メッシュ {}、クラスタ {}、配置 {}）を超えたので、このモデルは描きません", meshCapacity_, clusterCapacity_, placementCapacity_);
				capacityLogged_ = true;
			}
			return nullptr;
		}

		record.registered_ = true;
		record.generation_ = handle.generation_;
		record.crister_ = &crister;
		record.meshOffset_ = meshOffset;
		record.meshCount_ = meshTotal;
		record.clusterOffset_ = clusterOffset;
		record.clusterCount_ = clusterTotal;
		record.placementOffset_ = placementOffset;
		record.placementCount_ = placementTotal;
		record.surfaceCount_ = Max(static_cast<Uint32>(crister.Surfaces().size()), 1u);
		ModelClusterStructuredBuffer unsent;
		unsent.flags_ = SC_INVALID;
		record.clusters_.assign(clusterTotal, unsent);
		record.morphNodes_.assign(subMeshes.size(), -1);
		record.morphWeightOffsets_.assign(subMeshes.size(), 0);
		record.residencyVersion_ = std::numeric_limits<Uint64>::max();
		record.usedFrame_ = frame_;
		used_.push_back(recordIndex);

		const std::span<const Skin> skins = crister.Skins();
		for (const Skin& skin : skins)
		{
			record.boneCount_ += static_cast<Uint32>(skin.joints_.size());
		}

		const std::span<const Node> nodes = crister.Nodes();
		Uint32 placementIndex = placementOffset;
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
			mesh.placementOffset_ = placementIndex;
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
				placementUpdate.index_ = placementIndex;
				placementUpdate.data_ = placement;
				placementUpdates_.push_back(placementUpdate);
				++placementIndex;
			}
		}
		return &record;
	}

	void ModelMesh::Stream(std::span<const Uint32> streaming, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap& heap)
	{
		Uint32 geometryUploads = 0;
		Uint32 textureUploads = 0;
		for (Uint32 recordIndex : used_)
		{
			const ModelMeshRecord& record = records_[recordIndex];
			Crister& crister = *record.crister_;
			CristerResidency& residency = crister.Residency();
			const std::span<const SubMesh> subMeshes = crister.SubMeshes();
			const std::span<const Surface> surfaces = crister.Surfaces();
			for (Uint32 subMeshIndex = 0;subMeshIndex < record.meshCount_;++subMeshIndex)
			{
				Size row = static_cast<Size>(record.meshOffset_ + subMeshIndex) * 2;
				if (row + 1 >= streaming.size() || streaming[row] == SC_INVALID)
				{
					continue;
				}

				const SubMesh& subMesh = subMeshes[subMeshIndex];
				if (subMesh.clusterCount_ > 0)
				{
					Uint32 desiredCluster = subMesh.clusterOffset_ + Min(streaming[row], subMesh.clusterCount_ - 1);
					if (!residency.ClusterResident(desiredCluster) && geometryUploads < 4)
					{
						residency.MakeClusterResident(crister, device, lane, &heap, desiredCluster);
						++geometryUploads;
					}
					for (Uint32 clusterIndex = desiredCluster;clusterIndex < subMesh.clusterOffset_ + subMesh.clusterCount_;++clusterIndex)
					{
						if (residency.ClusterResident(clusterIndex))
						{
							residency.TouchCluster(clusterIndex, frame_);
							break;
						}
					}
				}

				if (surfaces.empty())
				{
					continue;
				}
				Float density = 0.0f;
				std::memcpy(&density, &streaming[row + 1], sizeof(Float));
				const Surface& surface = surfaces[subMesh.surfaceIndex_ < surfaces.size() ? subMesh.surfaceIndex_ : 0];
				StaticArray<Uint32, 17> textures =
				{
					surface.baseColorTextureAssetID_ != 0 ? SC_INVALID : surface.baseColorTextureIndex_,
					surface.normalTextureAssetID_ != 0 ? SC_INVALID : surface.normalTextureIndex_,
					surface.metallicRoughnessTextureAssetID_ != 0 ? SC_INVALID : surface.metallicRoughnessTextureIndex_,
					surface.emissiveTextureAssetID_ != 0 ? SC_INVALID : surface.emissiveTextureIndex_,
					surface.occlusionTextureAssetID_ != 0 ? SC_INVALID : surface.occlusionTextureIndex_,
					surface.khr_.specular_.specularTextureIndex_,
					surface.khr_.specular_.specularColorTextureIndex_,
					surface.khr_.clearCoat_.clearCoatTextureIndex_,
					surface.khr_.clearCoat_.clearCoatRoughnessTextureIndex_,
					surface.khr_.clearCoat_.clearCoatNormalTextureIndex_,
					surface.khr_.transmission_.transmissionTextureIndex_,
					surface.khr_.volume_.thicknessTextureIndex_,
					surface.khr_.sheen_.sheenColorTextureIndex_,
					surface.khr_.sheen_.sheenRoughnessTextureIndex_,
					surface.khr_.iridescence_.iridescenceTextureIndex_,
					surface.khr_.iridescence_.iridescenceThicknessTextureIndex_,
					surface.khr_.anisotropy_.anisotropyTextureIndex_
				};
				for (Uint32 textureIndex : textures)
				{
					if (textureIndex == SC_INVALID)
					{
						continue;
					}
					Uint32 desiredMip = residency.TextureDesiredMip(crister, textureIndex, subMesh.texcoordDensity_, density, 1.0f);
					if (residency.TextureFinestMip(textureIndex) > desiredMip && textureUploads < 8)
					{
						residency.MakeTextureMipResident(crister, device, lane, &heap, textureIndex, desiredMip);
						++textureUploads;
					}
					residency.TouchTexture(textureIndex, frame_);
				}
			}
		}
	}

	void ModelMesh::End(BindlessHeap& heap)
	{
		for (Uint32 recordIndex : used_)
		{
			ModelMeshRecord& record = records_[recordIndex];
			Crister& crister = *record.crister_;
			if (record.residencyVersion_ == crister.Residency().Version())
			{
				continue;
			}
			record.residencyVersion_ = crister.Residency().Version();

			const std::span<const Cluster> clusters = crister.Clusters();
			for (Uint32 clusterIndex = 0;clusterIndex < static_cast<Uint32>(clusters.size());++clusterIndex)
			{
				Bool resident = crister.Residency().ClusterResident(clusterIndex);
				ModelClusterStructuredBuffer cluster;
				cluster.meshletOffset_ = clusters[clusterIndex].meshletOffset_;
				cluster.meshletCount_ = clusters[clusterIndex].meshletCount_;
				cluster.lodError_ = clusters[clusterIndex].lodError_;
				cluster.flags_ = resident ? 1u : 0u;
				if (resident)
				{
					cluster.flags_ |= crister.Residency().StandaloneVertices(clusterIndex) ? 2u : 0u;
					cluster.vertexBufferIndex_ = crister.Residency().ClusterVertexBufferIndex(clusterIndex);
					cluster.meshletBufferIndex_ = crister.Residency().ClusterMeshletBufferIndex(clusterIndex);
					cluster.meshletBoundBufferIndex_ = crister.Residency().ClusterMeshletBoundBufferIndex(clusterIndex);
					cluster.vertexIndicesBufferIndex_ = crister.Residency().ClusterVertexIndicesBufferIndex(clusterIndex);
					cluster.primitiveIndicesBufferIndex_ = crister.Residency().ClusterPrimitiveIndicesBufferIndex(clusterIndex);
				}
				if (std::memcmp(&cluster, &record.clusters_[clusterIndex], sizeof(ModelClusterStructuredBuffer)) == 0)
				{
					continue;
				}
				record.clusters_[clusterIndex] = cluster;

				ModelClusterUpdate update;
				update.index_ = record.clusterOffset_ + clusterIndex;
				update.data_ = cluster;
				clusterUpdates_.push_back(update);
			}
		}

		for (ModelMeshRecord& record : records_)
		{
			if (record.registered_ && record.usedFrame_ != frame_)
			{
				Release(record);
			}
		}

		CristerResidency::EvictClusterBudget(&heap, frame_);
		CristerResidency::EvictTextureBudget(&heap, frame_);
	}

	Uint32 ModelMesh::Count()const
	{
		return meshCount_;
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

	Uint32 ModelMesh::Allocate(DynamicArray<Range>& freeRanges, Uint32& end, Uint32 count, Uint32 capacity)
	{
		if (count == 0)
		{
			return 0;
		}
		for (Size rangeIndex = 0;rangeIndex < freeRanges.size();++rangeIndex)
		{
			Range& range = freeRanges[rangeIndex];
			if (range.count_ < count)
			{
				continue;
			}
			Uint32 offset = range.offset_;
			range.offset_ += count;
			range.count_ -= count;
			if (range.count_ == 0)
			{
				freeRanges.erase(freeRanges.begin() + rangeIndex);
			}
			return offset;
		}
		if (end + count > capacity)
		{
			return SC_INVALID;
		}
		Uint32 offset = end;
		end += count;
		return offset;
	}

	void ModelMesh::Free(DynamicArray<Range>& freeRanges, Uint32& end, Uint32 offset, Uint32 count)
	{
		if (count == 0)
		{
			return;
		}
		auto next = std::ranges::lower_bound(freeRanges, offset, {}, &Range::offset_);
		Size rangeIndex = static_cast<Size>(next - freeRanges.begin());
		freeRanges.insert(next, Range{ offset, count });
		if (rangeIndex + 1 < freeRanges.size() && freeRanges[rangeIndex].offset_ + freeRanges[rangeIndex].count_ == freeRanges[rangeIndex + 1].offset_)
		{
			freeRanges[rangeIndex].count_ += freeRanges[rangeIndex + 1].count_;
			freeRanges.erase(freeRanges.begin() + rangeIndex + 1);
		}
		if (rangeIndex > 0 && freeRanges[rangeIndex - 1].offset_ + freeRanges[rangeIndex - 1].count_ == freeRanges[rangeIndex].offset_)
		{
			freeRanges[rangeIndex - 1].count_ += freeRanges[rangeIndex].count_;
			freeRanges.erase(freeRanges.begin() + rangeIndex);
			--rangeIndex;
		}
		if (freeRanges[rangeIndex].offset_ + freeRanges[rangeIndex].count_ == end)
		{
			end = freeRanges[rangeIndex].offset_;
			freeRanges.erase(freeRanges.begin() + rangeIndex);
		}
	}

	void ModelMesh::Release(ModelMeshRecord& record)
	{
		Free(freeMeshes_, meshCount_, record.meshOffset_, record.meshCount_);
		Free(freeClusters_, clusterCount_, record.clusterOffset_, record.clusterCount_);
		Free(freePlacements_, placementCount_, record.placementOffset_, record.placementCount_);
		record = ModelMeshRecord{};
	}
}
