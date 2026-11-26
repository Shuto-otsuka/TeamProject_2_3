#include <GraphicsEngine/Model/Transparent/Transparent.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>
#include <GraphicsEngine/Illumination/VolumetricLight/Froxel.hlsli>

struct TransparentClearBindings
{
	uint transparent_heads_;
	uint transparent_counter_;
};
ConstantBuffer<TransparentClearBindings> clear_bindings : register(b0, space1);

struct TransparentResolveBindings
{
	uint transparent_heads_;
	uint transparent_fragments_;
	uint raytracing_volumetric_light_fog_;
	uint radiance_color_;
	uint material_velocity_;
};
ConstantBuffer<TransparentResolveBindings> resolve_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Clear(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<uint> heads = ResourceDescriptorHeap[clear_bindings.transparent_heads_];
	uint width;
	uint height;
	heads.GetDimensions(width, height);
	if (id.x < width && id.y < height)
	{
		heads[id.xy] = 0xFFFFFFFF;
	}
	if (all(id.xy == 0))
	{
		RWStructuredBuffer<uint> counter = ResourceDescriptorHeap[clear_bindings.transparent_counter_];
		counter[0] = 0;
	}
}

[numthreads(8, 8, 1)]
void Resolve(uint3 id : SV_DispatchThreadID)
{
	Texture2D<uint> heads = ResourceDescriptorHeap[resolve_bindings.transparent_heads_];
	uint width;
	uint height;
	heads.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	uint node = heads.Load(int3(id.xy, 0));
	if (node == 0xFFFFFFFF)
	{
		return;
	}

	StructuredBuffer<TransparentFragmentStructuredBuffer> fragments = ResourceDescriptorHeap[resolve_bindings.transparent_fragments_];
	uint capacity;
	uint stride;
	fragments.GetDimensions(capacity, stride);

	float depths[16];
	uint indices[16];
	uint count = 0;
	uint walked = 0;
	while (node < capacity && walked < capacity)
	{
		TransparentFragmentStructuredBuffer fragment = fragments[node];
		walked++;
		if (count < 16 || fragment.depth_ > depths[15])
		{
			uint position = count < 16 ? count : 15;
			while (position > 0 && depths[position - 1] < fragment.depth_)
			{
				depths[position] = depths[position - 1];
				indices[position] = indices[position - 1];
				position--;
			}
			depths[position] = fragment.depth_;
			indices[position] = node;
			count = min(count + 1, 16);
		}
		node = fragment.next_;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	Texture3D<float4> fog_volume = ResourceDescriptorHeap[resolve_bindings.raytracing_volumetric_light_fog_];
	uint3 froxel_dimensions;
	fog_volume.GetDimensions(froxel_dimensions.x, froxel_dimensions.y, froxel_dimensions.z);
	float2 uv = (float2(id.xy) + 0.5) / float2(width, height);

	RWTexture2D<float4> radiance = ResourceDescriptorHeap[resolve_bindings.radiance_color_];
	float4 destination = radiance[id.xy];
	float3 color = destination.rgb;
	for (int layer = int(count) - 1; layer >= 0; layer--)
	{
		TransparentFragmentStructuredBuffer fragment = fragments[indices[layer]];
		float3 layer_color = float3(f16tof32(fragment.color_rg_), f16tof32(fragment.color_rg_ >> 16), f16tof32(fragment.color_ba_));
		float layer_alpha = f16tof32(fragment.color_ba_ >> 16);
		float slice = saturate(ViewZToFroxelSlice(scene.near_plane_ / max(fragment.depth_, 1e-7), scene.near_plane_, scene.far_plane_) - 0.5 / float(froxel_dimensions.z));
		float4 fog = fog_volume.SampleLevel(sampler_linear_clamp, float3(uv, slice), 0);
		color = layer_color * fog.a + fog.rgb * layer_alpha + color * (1.0 - layer_alpha);
	}
	radiance[id.xy] = float4(color, destination.a);

	float opacity = 0.0;
	for (uint front = 0; front < count; front++)
	{
		TransparentFragmentStructuredBuffer fragment = fragments[indices[front]];
		opacity += (1.0 - opacity) * f16tof32(fragment.color_ba_ >> 16);
		if (opacity > 0.5)
		{
			RWTexture2D<float2> velocity = ResourceDescriptorHeap[resolve_bindings.material_velocity_];
			velocity[id.xy] = float2(f16tof32(fragment.velocity_), f16tof32(fragment.velocity_ >> 16));
			break;
		}
	}
}
