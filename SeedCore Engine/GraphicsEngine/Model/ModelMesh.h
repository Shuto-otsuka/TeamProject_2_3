#pragma once
#include <GraphicsEngine/Model/ModelDriven.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	class BindlessHeap;
	class Crister;
	class D3D12TransferLane;

	struct ModelMeshRecord
	{
		Bool registered_ = false;

		Uint64 generation_ = 0;

		Crister* crister_ = nullptr;

		Uint32 meshOffset_ = 0;

		Uint32 meshCount_ = 0;

		Uint32 clusterOffset_ = 0;

		Uint32 clusterCount_ = 0;

		Uint32 placementOffset_ = 0;

		Uint32 placementCount_ = 0;

		Uint32 surfaceCount_ = 0;

		Uint32 boneCount_ = 0;

		Uint32 morphWeightCount_ = 0;

		DynamicArray<Int> morphNodes_;

		DynamicArray<Uint32> morphWeightOffsets_;

		DynamicArray<ModelClusterStructuredBuffer> clusters_;

		Uint64 residencyVersion_ = std::numeric_limits<Uint64>::max();

		Uint64 usedFrame_ = 0;
	};

	class ModelMesh :public NonTransferable
	{
	public:
		SC_CONSTEX Uint32 meshCapacity_ = 32768;

		SC_CONSTEX Uint32 clusterCapacity_ = 131072;

		SC_CONSTEX Uint32 placementCapacity_ = 131072;

	public:
		ModelMesh();

		~ModelMesh();

		void Begin(Uint64 frame);

		NODISCARD const ModelMeshRecord* Register(const Handle<Crister>& handle, Crister& crister);

		void Stream(std::span<const Uint32> streaming, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap& heap);

		void End(BindlessHeap& heap);

	public:
		NODISCARD Uint32 Count()const;

		NODISCARD std::span<const ModelMeshUpdate> MeshUpdates()const;

		NODISCARD std::span<const ModelClusterUpdate> ClusterUpdates()const;

		NODISCARD std::span<const ModelPlacementUpdate> PlacementUpdates()const;

	private:
		struct Range
		{
			Uint32 offset_ = 0;

			Uint32 count_ = 0;
		};

	private:
		NODISCARD static Uint32 Allocate(DynamicArray<Range>& freeRanges, Uint32& end, Uint32 count, Uint32 capacity);

		static void Free(DynamicArray<Range>& freeRanges, Uint32& end, Uint32 offset, Uint32 count);

		void Release(ModelMeshRecord& record);

	private:
		Uint64 frame_ = 0;

		DynamicArray<ModelMeshRecord> records_;

		DynamicArray<Uint32> used_;

		Uint32 meshCount_ = 0;

		Uint32 clusterCount_ = 0;

		Uint32 placementCount_ = 0;

		DynamicArray<Range> freeMeshes_;

		DynamicArray<Range> freeClusters_;

		DynamicArray<Range> freePlacements_;

		Bool capacityLogged_ = false;

		DynamicArray<ModelMeshUpdate> meshUpdates_;

		DynamicArray<ModelClusterUpdate> clusterUpdates_;

		DynamicArray<ModelPlacementUpdate> placementUpdates_;
	};
}
