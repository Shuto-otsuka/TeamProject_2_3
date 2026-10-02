#include "../Particle.hlsli"
#include "../../../Shader/Noise.hlsli"

// @SC_ZEPHYR_INCLUDE

#include "../ParticlePool.hlsli"

struct ParticleModuleConstantBuffer
{
    // @SC_ZEPHYR_MODULE_FIELD
};

ConstantBuffer<ParticleModuleConstantBuffer> GetParticleModuleConstantBuffer()
{
    ParticleDispatchBuffer dispatch_buffer = GetParticleDispatchBuffer();
    return ResourceDescriptorHeap[dispatch_buffer.module_index_];
}

void ApplyGenerateModuleSpawn(inout ParticleSeed seed, ParticleModuleConstantBuffer modules, ParticleConstantBuffer particle)
{
    // @SC_ZEPHYR_SPAWN_CALL
}

void ApplyGenerateModuleUpdate(inout ParticleSeed seed, ParticleModuleConstantBuffer modules, ParticleConstantBuffer particle)
{
    // @SC_ZEPHYR_UPDATE_CALL
}

[numthreads(64, 1, 1)]
void SpawnMain(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    ParticleConstantBuffer particle = GetParticleConstantBuffer();
    if (dispatch_thread_id.x >= particle.spawn_count_)
    {
        return;
    }

    RWStructuredBuffer<ParticleCounters> counters = GetParticleCounters();

    uint dead_slot_index;
    InterlockedAdd(counters[0].dead_count_, -1, dead_slot_index);
    if (dead_slot_index == 0)
    {
        InterlockedAdd(counters[0].dead_count_, 1);
        return;
    }

    uint particle_index = GetDeadList()[dead_slot_index - 1];

    ParticleSeed seed = (ParticleSeed)0;

    uint random_seed = WangHash(particle_index ^ asuint(particle.emitter_age_));
    float random_u = WangHash(random_seed) / 4294967295.0f;
    float random_v = WangHash(random_seed ^ 0x68bc21ebu) / 4294967295.0f;
    float cos_theta = random_u * 2.0f - 1.0f;
    float sin_theta = sqrt(1.0f - cos_theta * cos_theta);
    float phi = random_v * 6.2831853f;
    float3 direction = float3(sin_theta * cos(phi), cos_theta, sin_theta * sin(phi));

    seed.position_ = particle.emitter_to_world_[3].xyz;
    seed.rotation_ = float4(0.0f, 0.0f, 0.0f, 1.0f);
    seed.base_velocity_ = direction * 3.0f;
    seed.live_velocity_ = seed.base_velocity_;
    seed.base_color_ = float4(1.0f, 1.0f, 1.0f, 1.0f);
    seed.live_color_ = seed.base_color_;
    seed.base_size_ = float3(0.1f, 0.1f, 0.1f);
    seed.live_size_ = seed.base_size_;
    seed.lifetime_ = 2.0f;
    seed.seed_ = random_seed;

    ParticleModuleConstantBuffer modules = GetParticleModuleConstantBuffer();

    ApplyGenerateModuleSpawn(seed, modules, particle);

    GetParticleSeed()[particle_index] = seed;

    uint alive_write_index;
    InterlockedAdd(counters[0].alive_write_count_, 1, alive_write_index);
    GetAliveListWrite()[alive_write_index] = particle_index;
}

[numthreads(64, 1, 1)]
void UpdateMain(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    RWStructuredBuffer<ParticleCounters> counters = GetParticleCounters();
    if (dispatch_thread_id.x >= counters[0].alive_count_)
    {
        return;
    }

    uint particle_index = GetAliveListRead()[dispatch_thread_id.x];

    RWStructuredBuffer<ParticleSeed> particle_buffer = GetParticleSeed();
    ParticleSeed seed = particle_buffer[particle_index];

    ParticleConstantBuffer particle = GetParticleConstantBuffer();
    ParticleModuleConstantBuffer modules = GetParticleModuleConstantBuffer();

    seed.age_ += particle.emitter_delta_;

    ApplyGenerateModuleUpdate(seed, modules, particle);

    seed.position_ += seed.live_velocity_ * particle.emitter_delta_;

    if (seed.age_ >= seed.lifetime_)
    {
        uint dead_write_index;
        InterlockedAdd(counters[0].dead_count_, 1, dead_write_index);
        GetDeadList()[dead_write_index] = particle_index;
        return;
    }

    particle_buffer[particle_index] = seed;

    uint alive_write_index;
    InterlockedAdd(counters[0].alive_write_count_, 1, alive_write_index);
    GetAliveListWrite()[alive_write_index] = particle_index;
}