#include <GraphicsEngine/Effect/Zephyr/ZephyrParticleStorage.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>

namespace SeedCore
{
	ZephyrParticleStorage::ZephyrParticleStorage(ID3D12Device* device, BindlessHeap* heap, Uint32 capacity) :capacity_(capacity)
	{
		seeds_ = MakePtr<ReadWriteStructuredBuffer<ParticleSeed>>(device, heap, capacity);
		deadList_ = MakePtr<ReadWriteStructuredBuffer<Uint32>>(device, heap, capacity);
		aliveList_[0] = MakePtr<ReadWriteStructuredBuffer<Uint32>>(device, heap, capacity);
		aliveList_[1] = MakePtr<ReadWriteStructuredBuffer<Uint32>>(device, heap, capacity);
		counters_ = MakePtr<ReadWriteStructuredBuffer<ParticleCounters>>(device, heap, 1);
		constants_ = MakePtr<StaticConstantBuffer<ParticleConstantBuffer>>(device, heap);
		dispatch_ = MakePtr<StaticConstantBuffer<ParticleDispatchBuffer>>(device, heap);
		modules_ = MakePtr<DynamicConstantBuffer>(device, heap, 256);
	}

	void ZephyrParticleStorage::UploadModule(ID3D12Device* device, const void* data, Uint byteSize)
	{
		modules_->Update(device, data, byteSize);
	}

	void ZephyrParticleStorage::UploadDispatch(const ParticleConstantBuffer& constants)
	{
		constants_->Update(constants);

		ParticleDispatchBuffer dispatchBuffer{};
		dispatchBuffer.particleIndex_ = seeds_->UnorderedAccessViewIndex();
		dispatchBuffer.deadListIndex_ = deadList_->UnorderedAccessViewIndex();
		dispatchBuffer.aliveListReadIndex_ = aliveList_[aliveReadIndex_]->UnorderedAccessViewIndex();
		dispatchBuffer.aliveListWriteIndex_ = aliveList_[1 - aliveReadIndex_]->UnorderedAccessViewIndex();
		dispatchBuffer.counterIndex_ = counters_->UnorderedAccessViewIndex();
		dispatchBuffer.moduleIndex_ = modules_->Index();
		dispatchBuffer.metaIndex_ = constants_->Index();
		dispatch_->Update(dispatchBuffer);
	}

	void ZephyrParticleStorage::Swap()
	{
		aliveReadIndex_ = 1 - aliveReadIndex_;
	}

	Uint32 ZephyrParticleStorage::DispatchIndex()const
	{
		return dispatch_->Index();
	}

	Uint32 ZephyrParticleStorage::Capacity()const
	{
		return capacity_;
	}
}