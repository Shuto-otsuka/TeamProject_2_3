#include <GraphicsEngine/Effect/Zephyr/Quad/ParticleQuad.hlsli>

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, out vertices ParticleQuadOutput verts[128], out indices uint3 tris[64])
{
	StructuredBuffer<uint> offset_table = ResourceDescriptorHeap[quad_bindings.particle_quad_offsets_];
	StructuredBuffer<uint> offsets = ResourceDescriptorHeap[offset_table[0]];
	uint emitter_count;
	uint stride;
	offsets.GetDimensions(emitter_count, stride);

	StructuredBuffer<uint> counter_table = ResourceDescriptorHeap[quad_bindings.particle_counters_];
	StructuredBuffer<ParticleCounterStructuredBuffer> counters = ResourceDescriptorHeap[counter_table[0]];
	uint total = emitter_count > 0 ? offsets[emitter_count - 1] + counters[emitter_count - 1].alive_write_count_ : 0;

	uint first = gid * 32;
	uint particle_count = first < total ? min(32, total - first) : 0;
	SetMeshOutputCounts(particle_count * 4, particle_count * 2);
	if (particle_count == 0)
	{
		return;
	}

	uint particle_local = gtid / 4;
	if (particle_local < particle_count)
	{
		uint particle_number = first + particle_local;
		uint low = 0;
		uint high = emitter_count;
		while (low + 1 < high)
		{
			uint middle = (low + high) / 2;
			if (offsets[middle] <= particle_number)
			{
				low = middle;
			}
			else
			{
				high = middle;
			}
		}
		uint emitter_index = low;
		uint alive_index = particle_number - offsets[emitter_index];

		StructuredBuffer<ParticleEmitterStructuredBuffer> emitters = ResourceDescriptorHeap[quad_bindings.particle_emitters_];
		ParticleEmitterStructuredBuffer emitter = emitters[emitter_index];

		StructuredBuffer<uint> alive_table = ResourceDescriptorHeap[quad_bindings.particle_alive_];
		StructuredBuffer<uint> alive_list = ResourceDescriptorHeap[alive_table[emitter.particles_]];
		uint particle_index = alive_list[emitter.alive_write_ + alive_index];

		StructuredBuffer<uint> seed_table = ResourceDescriptorHeap[quad_bindings.particle_seeds_];
		StructuredBuffer<ParticleSeed> seeds = ResourceDescriptorHeap[seed_table[emitter.particles_]];
		ParticleSeed seed = seeds[emitter.base_ + particle_index];
		SceneConstantBuffer scene = GetSceneConstantBuffer();

		uint corner_index = gtid % 4;
		float2 offset = float2((corner_index & 1) != 0 ? 1.0 : -1.0, (corner_index & 2) != 0 ? -1.0 : 1.0);
		float3 world_position = seed.position_ + scene.inverse_view_[0].xyz * (offset.x * seed.live_size_.x * 0.5) + scene.inverse_view_[1].xyz * (offset.y * seed.live_size_.y * 0.5);

		ParticleQuadOutput output;
		output.position = mul(float4(world_position, 1.0), scene.current_view_projection_);
		output.local = offset;
		output.color = seed.live_color_;
		verts[gtid] = output;
	}

	if (gtid < particle_count)
	{
		uint base = gtid * 4;
		tris[gtid * 2] = uint3(base, base + 1, base + 2);
		tris[gtid * 2 + 1] = uint3(base + 1, base + 3, base + 2);
	}
}
