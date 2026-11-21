#include <GraphicsEngine/Shader/RootSignature.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>
#include <GraphicsEngine/Effect/Zephyr/Particle.hlsli>

#include <GraphicsEngine/Effect/Zephyr/Module/GravityModule.hlsli>
#include <GraphicsEngine/Effect/Zephyr/Module/DragModule.hlsli>


#include <GraphicsEngine/Effect/Zephyr/ParticlePool.hlsli>

struct ParticleSpawnBindings
{
	uint particle_emitters_;
	uint particle_parameters_;
	uint particle_seeds_;
	uint particle_dead_;
	uint particle_alive_;
	uint particle_counters_;
};
ConstantBuffer<ParticleSpawnBindings> spawn_bindings : register(b0, space1);

struct ParticleSimulateBindings
{
	uint particle_emitters_;
	uint particle_parameters_;
	uint particle_seeds_;
	uint particle_dead_;
	uint particle_alive_;
	uint particle_counters_;
	uint particle_offsets_;
	uint particle_simulate_arguments_;
};
ConstantBuffer<ParticleSimulateBindings> simulate_bindings : register(b0, space1);

void ApplyGenerateModuleSpawn(inout ParticleSeed seed, ParticleEmitter emitter, StructuredBuffer<float4> parameters, uint base)
{
	
}

void ApplyGenerateModuleSimulate(inout ParticleSeed seed, ParticleEmitter emitter, StructuredBuffer<float4> parameters, uint base)
{
		GravityModule gravity_0_;
	gravity_0_.gravity_ = parameters[base + 0].xyz;
	ApplyGravitySimulate(seed, gravity_0_, emitter);
	DragModule drag_1_;
	drag_1_.drag_ = parameters[base + 1].x;
	ApplyDragSimulate(seed, drag_1_, emitter);

}

[numthreads(64, 1, 1)]
void Spawn(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
	uint emitter_index = dispatch_bindings.constants_;
	StructuredBuffer<ParticleEmitterStructuredBuffer> emitters = ResourceDescriptorHeap[spawn_bindings.particle_emitters_];
	ParticleEmitterStructuredBuffer emitter = emitters[emitter_index];
	if (dispatch_thread_id.x >= emitter.spawn_count_)
	{
		return;
	}

	StructuredBuffer<uint> counter_table = ResourceDescriptorHeap[spawn_bindings.particle_counters_];
	RWStructuredBuffer<ParticleCounterStructuredBuffer> counters = ResourceDescriptorHeap[counter_table[0]];

	uint dead_slot_index;
	InterlockedAdd(counters[emitter_index].dead_count_, -1, dead_slot_index);
	if (dead_slot_index == 0)
	{
		InterlockedAdd(counters[emitter_index].dead_count_, 1);
		return;
	}

	StructuredBuffer<uint> dead_table = ResourceDescriptorHeap[spawn_bindings.particle_dead_];
	RWStructuredBuffer<uint> dead_list = ResourceDescriptorHeap[dead_table[emitter.particles_]];
	uint particle_index = dead_list[emitter.base_ + dead_slot_index - 1];

	ParticleSeed seed = (ParticleSeed)0;

	uint random_seed = WangHash(particle_index^asuint(emitter.emitter_.age_)^(emitter_index * 0x9e3779b9u));
	float random_u = WangHash(random_seed) / 4294967295.0;
	float random_v = WangHash(random_seed^0x68bc21ebu) / 4294967295.0;
	float cos_theta = random_u * 2.0 - 1.0;
	float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
	float phi = random_v * 6.2831853;
	float3 direction = float3(sin_theta * cos(phi), cos_theta, sin_theta * sin(phi));

	seed.position_ = emitter.emitter_.emitter_to_world_[3].xyz;
	seed.rotation_ = float4(0.0, 0.0, 0.0, 1.0);
	seed.base_velocity_ = direction * 3.0;
	seed.live_velocity_ = seed.base_velocity_;
	seed.base_color_ = float4(1.0, 1.0, 1.0, 1.0);
	seed.live_color_ = seed.base_color_;
	seed.base_size_ = float3(0.1, 0.1, 0.1);
	seed.live_size_ = seed.base_size_;
	seed.lifetime_ = 2.0;
	seed.seed_ = random_seed;

	StructuredBuffer<float4> parameters = ResourceDescriptorHeap[spawn_bindings.particle_parameters_];
	ApplyGenerateModuleSpawn(seed, emitter.emitter_, parameters, emitter.parameters_);

	StructuredBuffer<uint> seed_table = ResourceDescriptorHeap[spawn_bindings.particle_seeds_];
	RWStructuredBuffer<ParticleSeed> seeds = ResourceDescriptorHeap[seed_table[emitter.particles_]];
	seeds[emitter.base_ + particle_index] = seed;

	StructuredBuffer<uint> alive_table = ResourceDescriptorHeap[spawn_bindings.particle_alive_];
	RWStructuredBuffer<uint> alive_list = ResourceDescriptorHeap[alive_table[emitter.particles_]];
	uint alive_write_index;
	InterlockedAdd(counters[emitter_index].alive_write_count_, 1, alive_write_index);
	alive_list[emitter.alive_write_ + alive_write_index] = particle_index;
}

[numthreads(64, 1, 1)]
void Simulate(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
	StructuredBuffer<uint> offset_table = ResourceDescriptorHeap[simulate_bindings.particle_offsets_];
	StructuredBuffer<uint> offsets = ResourceDescriptorHeap[offset_table[0]];
	uint emitter_count;
	uint stride;
	offsets.GetDimensions(emitter_count, stride);

	StructuredBuffer<uint> counter_table = ResourceDescriptorHeap[simulate_bindings.particle_counters_];
	RWStructuredBuffer<ParticleCounterStructuredBuffer> counters = ResourceDescriptorHeap[counter_table[0]];
	if (emitter_count == 0 || dispatch_thread_id.x >= offsets[emitter_count - 1] + counters[emitter_count - 1].alive_count_)
	{
		return;
	}

	uint low = 0;
	uint high = emitter_count;
	while (low + 1 < high)
	{
		uint middle = (low + high) / 2;
		if (offsets[middle] <= dispatch_thread_id.x)
		{
			low = middle;
		}
		else
		{
			high = middle;
		}
	}
	uint emitter_index = low;
	uint alive_index = dispatch_thread_id.x - offsets[emitter_index];

	StructuredBuffer<ParticleEmitterStructuredBuffer> emitters = ResourceDescriptorHeap[simulate_bindings.particle_emitters_];
	ParticleEmitterStructuredBuffer emitter = emitters[emitter_index];

	StructuredBuffer<uint> alive_table = ResourceDescriptorHeap[simulate_bindings.particle_alive_];
	RWStructuredBuffer<uint> alive_list = ResourceDescriptorHeap[alive_table[emitter.particles_]];
	uint particle_index = alive_list[emitter.alive_read_ + alive_index];

	StructuredBuffer<uint> seed_table = ResourceDescriptorHeap[simulate_bindings.particle_seeds_];
	RWStructuredBuffer<ParticleSeed> seeds = ResourceDescriptorHeap[seed_table[emitter.particles_]];
	ParticleSeed seed = seeds[emitter.base_ + particle_index];

	seed.age_ += emitter.emitter_.delta_;

	StructuredBuffer<float4> parameters = ResourceDescriptorHeap[simulate_bindings.particle_parameters_];
	ApplyGenerateModuleSimulate(seed, emitter.emitter_, parameters, emitter.parameters_);

	seed.position_ += seed.live_velocity_ * emitter.emitter_.delta_;

	if (seed.age_ >= seed.lifetime_)
	{
		StructuredBuffer<uint> dead_table = ResourceDescriptorHeap[simulate_bindings.particle_dead_];
		RWStructuredBuffer<uint> dead_list = ResourceDescriptorHeap[dead_table[emitter.particles_]];
		uint dead_write_index;
		InterlockedAdd(counters[emitter_index].dead_count_, 1, dead_write_index);
		dead_list[emitter.base_ + dead_write_index] = particle_index;
		return;
	}

	seeds[emitter.base_ + particle_index] = seed;

	uint alive_write_index;
	InterlockedAdd(counters[emitter_index].alive_write_count_, 1, alive_write_index);
	alive_list[emitter.alive_write_ + alive_write_index] = particle_index;
}
