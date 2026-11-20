#ifndef __PARTICLE_QUAD_HLSL__
#define __PARTICLE_QUAD_HLSL__

#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Effect/Zephyr/ParticlePool.hlsli>

struct ParticleQuadBindings
{
	uint particle_emitters_;
	uint particle_seeds_;
	uint particle_alive_;
	uint particle_counters_;
	uint particle_quad_offsets_;
	uint particle_quad_arguments_;
	uint particle_quad_fallback_arguments_;
};
ConstantBuffer<ParticleQuadBindings> quad_bindings : register(b0, space1);

struct ParticleQuadOutput
{
	float4 position : SV_Position;
	float2 local : TEXCOORD0;
	float4 color : COLOR0;
};

#endif // __PARTICLE_QUAD_HLSL__
