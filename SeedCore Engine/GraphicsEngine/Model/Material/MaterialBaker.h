#pragma once
#include <GraphicsEngine/Model/Material/BakedSurface.h>

namespace SeedCore
{
	class Crister;
	class ResourceCache;
	class D3D12CommandQueue;
	class GraphicsProcessor;

	class SEEDCORE_API MaterialBaker
	{
	public:
		void BakeReferenced(ID3D12Device* device, D3D12CommandQueue* cmdQueue, GraphicsProcessor& processor, ResourceCache& cache, BakedSurface& bakedSurface)const;

		void BakeEmbedded(const Crister& crister, Uint32 modelAssetID, BakedSurface& bakedSurface)const;
	};
}