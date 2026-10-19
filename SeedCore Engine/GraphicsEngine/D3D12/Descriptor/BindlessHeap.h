#pragma once
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/D3D12/FrameRing.h>

namespace SeedCore
{
	class SEEDCORE_API BindlessHeap :public NonTransferable
	{
	public:
		BindlessHeap() = default;
		~BindlessHeap() = default;

		Bool Create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, Uint maxCount);

		NODISCARD Uint AllocateIndex();

		void Release(Microsoft::WRL::ComPtr<ID3D12Resource> resource, Uint index);

		void Release(Microsoft::WRL::ComPtr<ID3D12Resource> resource, std::initializer_list<Uint> indices);

		void Retire();

		NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle(Uint index)const;

		NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle(Uint index)const;

		NODISCARD Uint Index(D3D12_CPU_DESCRIPTOR_HANDLE handle)const;

		NODISCARD ID3D12DescriptorHeap* Heap()const;

	private:
		SC_CONSTEX Uint deferredSlotCount = FrameRing::frameCount + 1;

		std::mutex mutex_;

		ResourcePtr<DescriptorHeap> heap_;

		DynamicArray<Uint> freeLists_;

		DynamicArray<Uint> pendingIndices_[deferredSlotCount];

		DynamicArray<Microsoft::WRL::ComPtr<ID3D12Resource>> pendingResources_[deferredSlotCount];

		Uint deferredSlot_ = 0;

		Uint maxCount_ = 0;

		Bool exhaustedLogged_ = false;
	};
}