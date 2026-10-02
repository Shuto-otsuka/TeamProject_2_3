#include "ParticlePool.hlsli"

[numthreads(64, 1, 1)]
void InitializeMain(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    RWStructuredBuffer<uint> dead_list = GetDeadList();

    uint capacity;
    uint stride;
    dead_list.GetDimensions(capacity, stride);

    if (dispatch_thread_id.x >= capacity)
    {
        return;
    }

    dead_list[dispatch_thread_id.x] = dispatch_thread_id.x;

    if (dispatch_thread_id.x == 0)
    {
        RWStructuredBuffer<ParticleCounters> counters = GetParticleCounters();
        counters[0].alive_count_ = 0;
        counters[0].dead_count_ = capacity;
        counters[0].alive_write_count_ = 0;
    }
}

[numthreads(1, 1, 1)]
void PrepareMain(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    RWStructuredBuffer<ParticleCounters> counters = GetParticleCounters();
    counters[0].alive_count_ = counters[0].alive_write_count_;
    counters[0].alive_write_count_ = 0;
}