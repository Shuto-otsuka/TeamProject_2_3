#include <GraphicsEngine/Effect/Zephyr/Quad/ParticleQuad.hlsli>

ParticleQuadOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	ParticleQuadOutput output = (ParticleQuadOutput)0;
	output.position = float4(2.0, 2.0, 2.0, 1.0);

	StructuredBuffer<uint> offset_table = ResourceDescriptorHeap[quad_bindings.particle_quad_offsets_];
	StructuredBuffer<uint> offsets = ResourceDescriptorHeap[offset_table[0]];
	uint emitter_count;
	uint stride;
	offsets.GetDimensions(emitter_count, stride);
	if (emitter_count == 0)
	{
		return output;
	}

	uint low = 0;
	uint high = emitter_count;
	while (low + 1 < high)
	{
		uint middle = (low + high) / 2;
		if (offsets[middle] <= instance_id)
		{
			low = middle;
		}
		else
		{
			high = middle;
		}
	}
	uint emitter_index = low;
	uint alive_index = instance_id - offsets[emitter_index];

	StructuredBuffer<uint> counter_table = ResourceDescriptorHeap[quad_bindings.particle_counters_];
	StructuredBuffer<ParticleCounterStructuredBuffer> counters = ResourceDescriptorHeap[counter_table[0]];
	if (alive_index >= counters[emitter_index].alive_write_count_)
	{
		return output;
	}

	StructuredBuffer<ParticleEmitterStructuredBuffer> emitters = ResourceDescriptorHeap[quad_bindings.particle_emitters_];
	ParticleEmitterStructuredBuffer emitter = emitters[emitter_index];

	StructuredBuffer<uint> alive_table = ResourceDescriptorHeap[quad_bindings.particle_alive_];
	StructuredBuffer<uint> alive_list = ResourceDescriptorHeap[alive_table[emitter.particles_]];
	uint particle_index = alive_list[emitter.alive_write_ + alive_index];

	StructuredBuffer<uint> seed_table = ResourceDescriptorHeap[quad_bindings.particle_seeds_];
	StructuredBuffer<ParticleSeed> seeds = ResourceDescriptorHeap[seed_table[emitter.particles_]];
	ParticleSeed seed = seeds[emitter.base_ + particle_index];
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	uint corners[6] = { 0, 1, 2, 1, 3, 2 };
	uint corner_index = corners[vertex_id % 6];
	float2 offset = float2((corner_index & 1) != 0 ? 1.0 : -1.0, (corner_index & 2) != 0 ? -1.0 : 1.0);
	float3 world_position = seed.position_ + scene.inverse_view_[0].xyz * (offset.x * seed.live_size_.x * 0.5) + scene.inverse_view_[1].xyz * (offset.y * seed.live_size_.y * 0.5);

	output.position = mul(float4(world_position, 1.0), scene.current_view_projection_);
	output.local = offset;
	output.color = seed.live_color_;
	return output;
}
