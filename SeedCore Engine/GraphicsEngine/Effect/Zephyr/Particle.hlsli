#ifndef __PARTICLE_HLSL__
#define __PARTICLE_HLSL__

struct ParticleSeed
{
	float3 position_;
    float4 rotation_;
	
	float3 live_velocity_;
    float3 base_velocity_;
	
    float3 live_angular_velocity_;
    float3 base_angular_velocity_;
	
	float4 live_color_;
    float4 base_color_;
	
	float3 live_size_;
    float3 base_size_;
	
	float age_;
	float lifetime_;
	uint seed_;
};

struct ParticleEmitter
{
	float delta_;
	float age_;
	uint sim_space_;
	uint particle_emitter_padding_0_;

	row_major float4x4 emitter_to_world_;
	row_major float4x4 world_to_emitter_;
};

struct ParticleShape
{
	float3 position_;
	float3 normal_;
};

#endif // __PARTICLE_HLSL__