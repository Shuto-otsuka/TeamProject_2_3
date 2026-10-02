#ifndef __GRAVITY_MODULE_HLSL__
#define __GRAVITY_MODULE_HLSL__

#include "../Particle.hlsli"

struct GravityModule
{
	float3 gravity_;
};

void ApplyGravityUpdate(inout ParticleSeed seed, GravityModule module_, ParticleConstantBuffer particle)
{
	seed.live_velocity_ += module_.gravity_ * particle.emitter_delta_;
}

#endif // __GRAVITY_MODULE_HLSL__
