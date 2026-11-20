#include <GraphicsEngine/Shader/RootSignature.hlsli>
#include <GraphicsEngine/Effect/Zephyr/ParticlePool.hlsli>

struct ParticleDrivenBindings
{
	uint particle_counters_;
	uint particle_quad_offsets_;
	uint particle_quad_arguments_;
	uint particle_quad_fallback_arguments_;
};
ConstantBuffer<ParticleDrivenBindings> bindings : register(b0, space1);

groupshared uint wave_totals[64];
groupshared uint chunk_base;

[numthreads(256, 1, 1)]
void main(uint group_index : SV_GroupIndex)
{
	StructuredBuffer<uint> counter_table = ResourceDescriptorHeap[bindings.particle_counters_];
	StructuredBuffer<ParticleCounterStructuredBuffer> counters = ResourceDescriptorHeap[counter_table[0]];
	StructuredBuffer<uint> offset_table = ResourceDescriptorHeap[bindings.particle_quad_offsets_];
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
		uint count = emitter_index < emitter_count ? counters[emitter_index].alive_write_count_ : 0;

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

	if (group_index == 0)
	{
		RWStructuredBuffer<uint> quad_arguments = ResourceDescriptorHeap[bindings.particle_quad_arguments_];
		quad_arguments[0] = (chunk_base + 31) / 32;
		quad_arguments[1] = 1;
		quad_arguments[2] = 1;

		RWStructuredBuffer<uint> quad_fallback_arguments = ResourceDescriptorHeap[bindings.particle_quad_fallback_arguments_];
		quad_fallback_arguments[0] = 6;
		quad_fallback_arguments[1] = chunk_base;
		quad_fallback_arguments[2] = 0;
		quad_fallback_arguments[3] = 0;
	}
}
