#include <GraphicsEngine/Shader/RootSignature.hlsli>
#include <GraphicsEngine/Effect/Zephyr/ParticlePool.hlsli>

struct ParticleBeginBindings
{
	uint particle_counters_;
	uint particle_offsets_;
	uint particle_simulate_arguments_;
	uint particle_groups_;
};
ConstantBuffer<ParticleBeginBindings> bindings : register(b0, space1);

groupshared uint wave_totals[64];
groupshared uint chunk_base;

[numthreads(256, 1, 1)]
void main(uint group_index : SV_GroupIndex)
{
	StructuredBuffer<uint> counter_table = ResourceDescriptorHeap[bindings.particle_counters_];
	RWStructuredBuffer<ParticleCounterStructuredBuffer> counters = ResourceDescriptorHeap[counter_table[0]];
	StructuredBuffer<uint> offset_table = ResourceDescriptorHeap[bindings.particle_offsets_];
	RWStructuredBuffer<uint> offsets = ResourceDescriptorHeap[offset_table[0]];

	uint emitter_count;
	uint stride;
	counters.GetDimensions(emitter_count, stride);

	if (group_index == 0)
	{
		chunk_base = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	uint wave_index = group_index / WaveGetLaneCount();
	for (uint first = 0; first < emitter_count; first += 256)
	{
		uint emitter_index = first + group_index;
		uint count = 0;
		if (emitter_index < emitter_count)
		{
			count = counters[emitter_index].alive_write_count_;
			counters[emitter_index].alive_count_ = count;
			counters[emitter_index].alive_write_count_ = 0;
		}

		uint wave_prefix = WavePrefixSum(count);
		uint wave_total = WaveActiveSum(count);
		if (WaveIsFirstLane())
		{
			wave_totals[wave_index] = wave_total;
		}
		GroupMemoryBarrierWithGroupSync();

		uint offset = chunk_base + wave_prefix;
		for (uint previous = 0; previous < wave_index; ++previous)
		{
			offset += wave_totals[previous];
		}
		GroupMemoryBarrierWithGroupSync();

		if (emitter_index < emitter_count)
		{
			offsets[emitter_index] = offset;
		}
		if (group_index == 255)
		{
			chunk_base = offset + count;
		}
		GroupMemoryBarrierWithGroupSync();
	}

	DeviceMemoryBarrierWithGroupSync();

	StructuredBuffer<ParticleGroupStructuredBuffer> groups = ResourceDescriptorHeap[bindings.particle_groups_];
	RWStructuredBuffer<uint> arguments = ResourceDescriptorHeap[bindings.particle_simulate_arguments_];
	uint group_count = dispatch_bindings.constants_;
	for (uint particle_group_index = group_index; particle_group_index < group_count; particle_group_index += 256)
	{
		ParticleGroupStructuredBuffer group = groups[particle_group_index];
		uint total = 0;
		if (group.count_ > 0)
		{
			uint last = group.first_ + group.count_ - 1;
			total = offsets[last] + counters[last].alive_count_ - offsets[group.first_];
		}
		arguments[particle_group_index * 3 + 0] = (total + 63) / 64;
		arguments[particle_group_index * 3 + 1] = 1;
		arguments[particle_group_index * 3 + 2] = 1;
	}
}
