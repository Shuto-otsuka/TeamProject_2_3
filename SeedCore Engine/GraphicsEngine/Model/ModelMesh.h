#pragma once
#include <GraphicsEngine/Model/ModelDriven.h>

namespace SeedCore
{
	class BindlessHeap;
	class Crister;

	struct ModelMeshRecord
	{
		Uint32 meshOffset_ = 0;

		Uint32 meshCount_ = 0;

		Uint32 clusterOffset_ = 0;

		Uint32 surfaceCount_ = 0;

		Uint32 boneCount_ = 0;

		Uint32 morphWeightCount_ = 0;

		DynamicArray<Int> morphNodes_;

		DynamicArray<Uint32> morphWeightOffsets_;

		DynamicArray<Uint32> clusterFlags_;

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

		NODISCARD const ModelMeshRecord* Register(Crister& crister);

		void End(BindlessHeap& heap);

	public:
		NODISCARD std::span<const ModelMeshUpdate> MeshUpdates()const;

		NODISCARD std::span<const ModelClusterUpdate> ClusterUpdates()const;

		NODISCARD std::span<const ModelPlacementUpdate> PlacementUpdates()const;

	private:
		Uint64 frame_ = 0;

		std::unordered_map<Crister*, ModelMeshRecord> records_;

		Uint32 meshCount_ = 0;

		Uint32 clusterCount_ = 0;

		Uint32 placementCount_ = 0;

		Bool capacityLogged_ = false;

		DynamicArray<ModelMeshUpdate> meshUpdates_;

		DynamicArray<ModelClusterUpdate> clusterUpdates_;

		DynamicArray<ModelPlacementUpdate> placementUpdates_;
	};
}
