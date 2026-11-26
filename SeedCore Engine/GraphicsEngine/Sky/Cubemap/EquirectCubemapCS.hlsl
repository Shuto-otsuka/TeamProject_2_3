#include <GraphicsEngine/Sky/Sky.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct SkyEquirectBindings
{
	uint sky_cubemap_;
};
ConstantBuffer<SkyEquirectBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	RWTexture2DArray<float4> destination = ResourceDescriptorHeap[bindings.sky_cubemap_];
	uint width;
	uint height;
	uint element_count;
	destination.GetDimensions(width, height, element_count);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	float4 color = float4(0.0, 0.0, 0.0, 1.0);
	if (dispatch_bindings.constants_ != 0xFFFFFFFF)
	{
		Texture2D<float4> source = ResourceDescriptorHeap[dispatch_bindings.constants_];
		uint source_width;
		uint source_height;
		source.GetDimensions(source_width, source_height);
		uint sample_count = clamp((uint)ceil(float(source_width) / float(width * 4)), 1u, 8u);

		float4 sum = float4(0.0, 0.0, 0.0, 0.0);
		for (uint sample_y = 0; sample_y < sample_count; sample_y++)
		{
			for (uint sample_x = 0; sample_x < sample_count; sample_x++)
			{
				float2 texel = float2(id.xy) + (float2(sample_x, sample_y) + 0.5) / float(sample_count);
				float2 uv = texel / float(width) * 2.0 - 1.0;
				sum += source.SampleLevel(sampler_linear_clamp, EquirectangularUv(CubeFaceDirection(id.z, uv)), 0);
			}
		}
		color = sum / float(sample_count * sample_count);
	}
	destination[id] = color;
}
