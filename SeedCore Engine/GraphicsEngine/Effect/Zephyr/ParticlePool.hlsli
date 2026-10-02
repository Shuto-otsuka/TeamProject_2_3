#ifndef __PARTICLE_POOL_HLSL__
#define __PARTICLE_POOL_HLSL__

#include "../../Shader/Dispatch.hlsli"
#include "Particle.hlsli"

struct ParticleDispatchBuffer
{
    uint particle_index_;
    uint dead_list_index_;
    uint alive_list_read_index_;
    uint alive_list_write_index_;

    uint counter_index_;
    uint module_index_;
    uint meta_index_;
    uint particle_dispatch_buffer_padding_0_;
};

ConstantBuffer<ParticleDispatchBuffer> GetParticleDispatchBuffer()
{
    return ResourceDescriptorHeap[dispatch_buffer_index_];
}

ConstantBuffer<ParticleConstantBuffer> GetParticleConstantBuffer()
{
    ParticleDispatchBuffer dispatchBuffer = GetParticleDispatchBuffer();
    return ResourceDescriptorHeap[dispatchBuffer.meta_index_];
}

RWStructuredBuffer<ParticleSeed> GetParticleSeed()
{
    ParticleDispatchBuffer dispatchBuffer = GetParticleDispatchBuffer();
    return ResourceDescriptorHeap[dispatchBuffer.particle_index_];
}

struct ParticleCounters
{
	uint alive_count_;
	uint dead_count_;
	uint alive_write_count_;
	uint particle_counters_padding_0_;
};

RWStructuredBuffer<ParticleCounters> GetParticleCounters()
{
	ParticleDispatchBuffer dispatchBuffer = GetParticleDispatchBuffer();
	return ResourceDescriptorHeap[dispatchBuffer.counter_index_];
}

RWStructuredBuffer<uint> GetDeadList()
{
	ParticleDispatchBuffer dispatchBuffer = GetParticleDispatchBuffer();
	return ResourceDescriptorHeap[dispatchBuffer.dead_list_index_];
}

RWStructuredBuffer<uint> GetAliveListRead()
{
	ParticleDispatchBuffer dispatchBuffer = GetParticleDispatchBuffer();
	return ResourceDescriptorHeap[dispatchBuffer.alive_list_read_index_];
}

RWStructuredBuffer<uint> GetAliveListWrite()
{
	ParticleDispatchBuffer dispatchBuffer = GetParticleDispatchBuffer();
	return ResourceDescriptorHeap[dispatchBuffer.alive_list_write_index_];
}

#endif // __PARTICLE_POOL_HLSL__