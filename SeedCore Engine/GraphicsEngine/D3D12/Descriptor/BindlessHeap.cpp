#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	Bool BindlessHeap::Create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, Uint maxCount)
	{
		heap_ = MakePtr<DescriptorHeap>();
		if (!heap_->Create(device, type, maxCount, true))
		{
			return false;
		}

		maxCount_ = maxCount;
		freeLists_.reserve(maxCount);

		for (Int index = static_cast<Int>(maxCount_) - 1; index >= 0; --index)
		{
			freeLists_.push_back(static_cast<Uint>(index));
		}

		return true;
	}

	Uint BindlessHeap::AllocateIndex()
	{
		std::scoped_lock lock(mutex_);

		if (freeLists_.empty())
		{
			if (!exhaustedLogged_)
			{
				SC_LOG_ERROR("バインドレスディスクリプタヒープが枯渇しました(上限 {})。以降の確保はディスクリプタ 0 を共有するため描画が乱れます。", maxCount_);
				exhaustedLogged_ = true;
			}
			return 0;
		}

		Uint index = freeLists_.back();
		freeLists_.pop_back();
		return index;
	}

	void BindlessHeap::Release(Microsoft::WRL::ComPtr<ID3D12Resource> resource, Uint index)
	{
		std::scoped_lock lock(mutex_);

		if (index != SC_INVALID)
		{
			pendingIndices_[deferredSlot_].push_back(index);
		}

		if (resource)
		{
			pendingResources_[deferredSlot_].push_back(std::move(resource));
		}
	}

	void BindlessHeap::Release(Microsoft::WRL::ComPtr<ID3D12Resource> resource, std::initializer_list<Uint> indices)
	{
		std::scoped_lock lock(mutex_);

		for (Uint index : indices)
		{
			if (index != SC_INVALID)
			{
				pendingIndices_[deferredSlot_].push_back(index);
			}
		}

		if (resource)
		{
			pendingResources_[deferredSlot_].push_back(std::move(resource));
		}
	}

	void BindlessHeap::Retire()
	{
		std::scoped_lock lock(mutex_);

		deferredSlot_ = (deferredSlot_ + 1) % deferredSlotCount;

		for (Uint index : pendingIndices_[deferredSlot_])
		{
			freeLists_.push_back(index);
		}
		pendingIndices_[deferredSlot_].clear();

		pendingResources_[deferredSlot_].clear();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE BindlessHeap::CPUHandle(Uint index)const
	{
		return heap_->CPUHandle(index);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE BindlessHeap::GPUHandle(Uint index)const
	{
		return heap_->GPUHandle(index);
	}

	Uint BindlessHeap::Index(D3D12_CPU_DESCRIPTOR_HANDLE handle)const
	{
		return static_cast<Uint>((handle.ptr - heap_->CPUHandle(0).ptr) / heap_->IncrementSize());
	}

	ID3D12DescriptorHeap* BindlessHeap::Heap()const
	{
		return heap_->Get();
	}
}