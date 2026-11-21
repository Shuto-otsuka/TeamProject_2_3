#ifndef __GRAVITY_MODULE_HLSL__
#define __GRAVITY_MODULE_HLSL__

#include <GraphicsEngine/Effect/Zephyr/Particle.hlsli>

struct GravityModule
{
	float3 gravity_;
};

void ApplyGravitySimulate(inout ParticleSeed seed, GravityModule module_, ParticleEmitter emitter)
{
	seed.live_velocity_ += module_.gravity_ * emitter.delta_;
}

#endif // __GRAVITY_MODULE_HLSL__
