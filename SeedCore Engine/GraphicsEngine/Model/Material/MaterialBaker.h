#pragma once
#include <GraphicsEngine/Model/Material/BakedSurface.h>

namespace SeedCore
{
	class Crister;
	class ResourceCache;
	class D3D12CommandQueue;
	class BC7CompressShader;

	class SEEDCORE_API MaterialBaker
	{
	public:
		void BakeReferenced(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BC7CompressShader& bc7Shader, ResourceCache& cache, BakedSurface& bakedSurface)const;

		void BakeEmbedded(const Crister& crister, Uint32 modelAssetID, BakedSurface& bakedSurface)const;
	};
}