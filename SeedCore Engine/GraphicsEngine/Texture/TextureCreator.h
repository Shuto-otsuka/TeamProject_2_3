#pragma once
#include <GraphicsEngine/Texture/Texture.h>

namespace SeedCore
{
	struct Bitmap;

	class BindlessHeap;
	class D3D12CommandQueue;
	class D3D12TransferLane;

	class SEEDCORE_API TextureCreator
	{
	public:
		static void CreateTexturePath(SC_IN ID3D12Device* device, SC_IN D3D12TransferLane& lane, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		static void CreateTexturePath(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* copyQueue, SC_IN D3D12TransferLane* transferLane, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		static void CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN D3D12TransferLane& lane, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		static void CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* copyQueue, SC_IN D3D12TransferLane* transferLane, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		static Texture CreateTextureBitmap(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* copyQueue, SC_IN D3D12TransferLane* transferLane, SC_IN BindlessHeap* heap, SC_IN const Bitmap& bitmap, SC_IN DXGI_FORMAT format = DXGI_FORMAT_BC7_UNORM);
	};
}
