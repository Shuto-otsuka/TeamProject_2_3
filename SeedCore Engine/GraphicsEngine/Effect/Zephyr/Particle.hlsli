#ifndef __PARTICLE_HLSL__
#define __PARTICLE_HLSL__

struct ParticleConstantBuffer
{
	float emitter_delta_;
	float emitter_age_;
    uint sim_space_;
    uint spawn_count_;
	row_major float4x4 emitter_to_world_;
	row_major float4x4 world_to_emitter_;
};

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

#endif // __PARTICLE_HLSL__