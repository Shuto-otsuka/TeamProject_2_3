#pragma once

namespace SeedCore
{
	class World;
	class MovieResource;
	class BindlessHeap;

	class MovieSystem
	{
	public:
		void Update(MovieResource& movieResource, World& world);

		void Upload(MovieResource& movieResource, ID3D12Device* device, ID3D12GraphicsCommandList6* cmdList, BindlessHeap* bindlessHeap);
	};
}
