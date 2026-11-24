#include <GraphicsEngine/Shader/RootSignature.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>
#include <GraphicsEngine/Effect/Zephyr/Particle.hlsli>

#include <GraphicsEngine/Effect/Zephyr/Module/ConeModule.hlsli>
#include <GraphicsEngine/Effect/Zephyr/Module/GravityModule.hlsli>
#include <GraphicsEngine/Effect/Zephyr/Module/RandomRangeModule.hlsli>


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
	uint particle_groups_;
};
ConstantBuffer<ParticleSimulateBindings> simulate_bindings : register(b0, space1);

void ApplyGenerateModuleSpawn(inout ParticleSeed seed, ParticleEmitter emitter, StructuredBuffer<float4> parameters, uint base)
{
		ConeInputs cone_7_;
	cone_7_.angle_ = parameters[base + 0].x;
	cone_7_.radius_ = parameters[base + 1].x;
	cone_7_.length_ = parameters[base + 2].x;
	ParticleShape shape = Cone(seed.seed_, cone_7_);
	seed.position_ = mul(float4(shape.position_, 1.0), emitter.emitter_to_world_).xyz;
	seed.lifetime_ = parameters[base + 3].x;
	seed.base_velocity_ = normalize(mul(float4(shape.normal_, 0.0), emitter.emitter_to_world_).xyz) * length(parameters[base + 4].xyz);
	seed.live_velocity_ = seed.base_velocity_;
	seed.base_color_ = parameters[base + 5];
	seed.live_color_ = seed.base_color_;
	seed.base_size_ = parameters[base + 6].xyz;
	seed.live_size_ = seed.base_size_;

}

void ApplyGenerateModuleSimulate(inout ParticleSeed seed, ParticleEmitter emitter, StructuredBuffer<float4> parameters, uint base)
{
	float3 force = float3(0.0, 0.0, 0.0);
		GravityInputs gravity_8_;
	RandomRangeInputs randomRange_5_;
	randomRange_5_.min_ = parameters[base + 7].xyz;
	randomRange_5_.max_ = parameters[base + 8].xyz;
	float3 randomRange_5_result_ = RandomRange(seed.seed_, randomRange_5_);
	gravity_8_.gravity_ = randomRange_5_result_;
	force += Gravity(seed, gravity_8_, emitter);

	seed.live_velocity_ += force * emitter.delta_;
	seed.position_ += seed.live_velocity_ * emitter.delta_;
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
	seed.position_ = emitter.emitter_.emitter_to_world_[3].xyz;
	seed.rotation_ = float4(0.0, 0.0, 0.0, 1.0);
	seed.seed_ = WangHash(particle_index ^ asuint(emitter.emitter_.age_) ^ (emitter_index * 0x9e3779b9u));

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
	StructuredBuffer<ParticleGroupStructuredBuffer> groups = ResourceDescriptorHeap[simulate_bindings.particle_groups_];
	ParticleGroupStructuredBuffer group = groups[dispatch_bindings.constants_];
	if (group.count_ == 0)
	{
		return;
	}

	StructuredBuffer<uint> offset_table = ResourceDescriptorHeap[simulate_bindings.particle_offsets_];
	StructuredBuffer<uint> offsets = ResourceDescriptorHeap[offset_table[0]];

	StructuredBuffer<uint> counter_table = ResourceDescriptorHeap[simulate_bindings.particle_counters_];
	RWStructuredBuffer<ParticleCounterStructuredBuffer> counters = ResourceDescriptorHeap[counter_table[0]];

	uint last = group.first_ + group.count_ - 1;
	uint thread_index = offsets[group.first_] + dispatch_thread_id.x;
	if (thread_index >= offsets[last] + counters[last].alive_count_)
	{
		return;
	}

	uint low = group.first_;
	uint high = group.first_ + group.count_;
	while (low + 1 < high)
	{
		uint middle = (low + high) / 2;
		if (offsets[middle] <= thread_index)
		{
			low = middle;
		}
		else
		{
			high = middle;
		}
	}
	uint emitter_index = low;
	uint alive_index = thread_index - offsets[emitter_index];

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
