#ifndef __DRAG_MODULE_HLSL__
#define __DRAG_MODULE_HLSL__

#include <GraphicsEngine/Effect/Zephyr/Particle.hlsli>

struct DragModule
{
	float drag_;
};

void ApplyDragSimulate(inout ParticleSeed seed, DragModule module_, ParticleEmitter emitter)
{
	seed.live_velocity_ *= max(0.0f, 1.0f - module_.drag_ * emitter.delta_);
}

#endif // __DRAG_MODULE_HLSL__
