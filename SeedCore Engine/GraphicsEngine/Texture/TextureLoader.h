#pragma once
#include <GraphicsEngine/Texture/Texture.h>
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Pool/StablePool.h>

namespace SeedCore
{
	class BindlessHeap;
	class D3D12CommandQueue;

	class SEEDCORE_API TextureLoader :public NonCopyable
	{
	public:
		TextureLoader() = default;
		~TextureLoader() = default;

		Handle<Texture> Load(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BindlessHeap* heap, String filePath);

		Texture* Get(const Handle<Texture>& handle);

		void Clear(Handle<Texture>& handle, BindlessHeap* heap)noexcept;

		Texture* Resolve(BindlessHeap* heap, const Handle<Texture>& handle, Uint64 frame);

		void EvictBudget(BindlessHeap* heap, Uint64 currentFrame);

	public:
		static void CreateTexturePath(SC_IN ID3D12Device* device, SC_IN DirectX::ResourceUploadBatch& resourceUpload, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);
		
		static void CreateTexturePath(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* cmdQueue, SC_IN ID3D12DescriptorHeap* heap, SC_IN String filePath, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		static void CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN DirectX::ResourceUploadBatch& resourceUpload, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

		static void CreateTextureMemory(SC_IN ID3D12Device* device, SC_IN D3D12CommandQueue* cmdQueue, SC_IN ID3D12DescriptorHeap* heap, SC_IN const DynamicArray<Byte>& data, SC_INOUT Microsoft::WRL::ComPtr<ID3D12Resource>& resource, SC_IN Uint textureIndex);

	private:
		StablePool<Texture> pool_;
		DynamicArray<Handle<Texture>> loadedHandles_;

		ID3D12Device* device_ = nullptr;
		D3D12CommandQueue* cmdQueue_ = nullptr;

		Uint64 totalResidentBytes_ = 0;
		Uint64 budgetBytes_ = 128ull * 1024 * 1024;
		SC_CONSTEX Uint64 evictAgeFrames_ = 8;
	};
}
