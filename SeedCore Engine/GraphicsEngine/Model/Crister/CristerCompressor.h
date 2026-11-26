#pragma once
#include <GraphicsEngine/Model/Crister/CristerMaterial.h>

namespace SeedCore
{
	class D3D12CommandQueue;
	class GraphicsProcessor;

	class SEEDCORE_API CristerCompressor
	{
	public:
		void BakeBitmap(ID3D12Device* device, D3D12CommandQueue* cmdQueue, GraphicsProcessor& processor, std::span<Bitmap> bitmaps)const;
	};
}