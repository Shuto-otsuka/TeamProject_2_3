#pragma once

namespace SeedCore
{
	class FontResource;
	class BindlessHeap;
	class D3D12TransferLane;

	class FontSystem
	{
	public:
		void Update(FontResource& fontResource, ID3D12Device* device, D3D12TransferLane& lane, BindlessHeap* bindlessHeap);
	};
}
