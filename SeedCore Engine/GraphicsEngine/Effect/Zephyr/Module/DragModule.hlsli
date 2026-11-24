// @node Drag
// @category 動き
// @output force
// @input float drag = 1.0

#ifndef __DRAG_MODULE_HLSL__
#define __DRAG_MODULE_HLSL__

#include <GraphicsEngine/Effect/Zephyr/Particle.hlsli>

struct DragInputs
{
	float drag_;
};

float3 Drag(ParticleSeed seed, DragInputs inputs, ParticleEmitter emitter)
{
	return -inputs.drag_ * seed.live_velocity_;
}

#endif // __DRAG_MODULE_HLSL__
