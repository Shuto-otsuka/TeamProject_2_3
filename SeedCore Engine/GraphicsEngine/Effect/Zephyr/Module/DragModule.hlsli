#ifndef __DRAG_MODULE_HLSL__
#define __DRAG_MODULE_HLSL__

#include "../Particle.hlsli"

struct DragModule
{
	float drag_;
};

void ApplyDragUpdate(inout ParticleSeed seed, DragModule module_, ParticleConstantBuffer particle)
{
	seed.live_velocity_ *= max(0.0f, 1.0f - module_.drag_ * particle.emitter_delta_);
}

#endif // __DRAG_MODULE_HLSL__
