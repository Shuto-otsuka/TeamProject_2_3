// @node Gravity
// @category 動き
// @output force
// @input float3 gravity = (0.0, -9.8, 0.0)

#ifndef __GRAVITY_MODULE_HLSL__
#define __GRAVITY_MODULE_HLSL__

#include <GraphicsEngine/Effect/Zephyr/Particle.hlsli>

struct GravityInputs
{
	float3 gravity_;
};

float3 Gravity(ParticleSeed seed, GravityInputs inputs, ParticleEmitter emitter)
{
	return inputs.gravity_;
}

#endif // __GRAVITY_MODULE_HLSL__
