#include <GraphicsEngine/Shader/RootSignature.hlsli>
#include <GraphicsEngine/Effect/Zephyr/ParticlePool.hlsli>

struct ParticleResetBindings
{
    uint particle_emitters_;
    uint particle_dead_;
    uint particle_counters_;
};
ConstantBuffer<ParticleResetBindings> bindings : register(b0, space1);

[numthreads(64, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint emitter_index = dispatch_bindings.constants_;
    StructuredBuffer<ParticleEmitterStructuredBuffer> emitters = ResourceDescriptorHeap[bindings.particle_emitters_];
    ParticleEmitterStructuredBuffer emitter = emitters[emitter_index];
    if (dispatch_thread_id.x >= emitter.capacity_)
    {
        return;
    }

    StructuredBuffer<uint> dead_table = ResourceDescriptorHeap[bindings.particle_dead_];
    RWStructuredBuffer<uint> dead_list = ResourceDescriptorHeap[dead_table[emitter.particles_]];
    dead_list[emitter.base_ + dispatch_thread_id.x] = dispatch_thread_id.x;

    if (dispatch_thread_id.x == 0)
    {
        StructuredBuffer<uint> counter_table = ResourceDescriptorHeap[bindings.particle_counters_];
        RWStructuredBuffer<ParticleCounterStructuredBuffer> counters = ResourceDescriptorHeap[counter_table[0]];
        counters[emitter_index].alive_count_ = 0;
        counters[emitter_index].dead_count_ = emitter.capacity_;
        counters[emitter_index].alive_write_count_ = 0;
    }
}