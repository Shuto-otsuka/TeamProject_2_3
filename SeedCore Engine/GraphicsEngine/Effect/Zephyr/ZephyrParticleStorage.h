#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/Effect/Zephyr/ZephyParticler.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/Buffer/StructuredBuffer.h>
#include <FoundationEngine/Log/Assert.h>

namespace SeedCore
{
	struct ParticleConstantBuffer
	{
		Float emitterDelta_ = 0.0f;
		Float emitterAge_ = 0.0f;
		Uint32 simSpace_ = 0;
		Uint32 spawnCount_ = 0;
		Matrix emitterToWorld_ = Matrix::Identity;
		Matrix worldToEmitter_ = Matrix::Identity;
	};
	SC_STATIC_ASSERT(ParticleConstantBuffer, 144, "Effect/Zephyr/Particle.hlsli");

	struct ParticleDispatchBuffer
	{
		Uint32 particleIndex_ = 0;
		Uint32 deadListIndex_ = 0;
		Uint32 aliveListReadIndex_ = 0;
		Uint32 aliveListWriteIndex_ = 0;

		Uint32 counterIndex_ = 0;
		Uint32 moduleIndex_ = 0;
		Uint32 metaIndex_ = 0;
		Uint32 particleDispatchBufferPadding0_;
	};
	SC_STATIC_ASSERT(ParticleDispatchBuffer, 32, "Effect/Zephyr/ParticlePool.hlsli");

	class ZephyrParticleStorage :public NonCopyable
	{
	public:
		ZephyrParticleStorage(ID3D12Device* device, BindlessHeap* heap, Uint32 capacity);

		void UploadModule(ID3D12Device* device, const void* data, Uint byteSize);

		void UploadDispatch(const ParticleConstantBuffer& constants);

	public:
		void Swap();

		[[nodiscard]] Uint32 DispatchIndex()const;

		[[nodiscard]] Uint32 Capacity()const;

	private:
		ResourcePtr<ReadWriteStructuredBuffer<ParticleSeed>> seeds_;

		ResourcePtr<ReadWriteStructuredBuffer<Uint32>> deadList_;

		ResourcePtr<ReadWriteStructuredBuffer<Uint32>> aliveList_[2];

		ResourcePtr<ReadWriteStructuredBuffer<ParticleCounters>> counters_;

		ResourcePtr<StaticConstantBuffer<ParticleConstantBuffer>> constants_;

		ResourcePtr<StaticConstantBuffer<ParticleDispatchBuffer>> dispatch_;

		ResourcePtr<DynamicConstantBuffer> modules_;

		Uint32 capacity_ = 0;
		Uint32 aliveReadIndex_ = 0;
	};
}