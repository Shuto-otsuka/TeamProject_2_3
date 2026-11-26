#ifndef __PARTICLE_POOL_HLSL__
#define __PARTICLE_POOL_HLSL__

#include <GraphicsEngine/Effect/Zephyr/Particle.hlsli>

struct ParticleEmitterStructuredBuffer
{
	uint particles_;
	uint base_;
	uint capacity_;
	uint alive_read_;

	uint alive_write_;
	uint parameters_;
	uint spawn_count_;
	uint particle_emitter_structured_buffer_padding_0_;

	ParticleEmitter emitter_;
};

struct ParticleCounterStructuredBuffer
{
	uint alive_count_;
	uint dead_count_;
	uint alive_write_count_;
	uint particle_counter_structured_buffer_padding_0_;
};

struct ParticleGroupStructuredBuffer
{
	uint first_;
	uint count_;
};

#endif