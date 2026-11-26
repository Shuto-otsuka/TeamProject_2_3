#include <GraphicsEngine/Shader/Scene.hlsli>

struct ChromaticAberrationConstantBuffer
{
	float intensity_;
	uint sample_count_;
	float2 chromatic_aberration_padding_0_;
};

struct ChromaticAberrationResolveBindings
{
	uint lens_distortion_color_;
	uint chromatic_aberration_constants_;
	uint chromatic_aberration_color_;
};
ConstantBuffer<ChromaticAberrationResolveBindings> bindings : register(b0, space1);

float3 SpectralResponse(float t)
{
	return saturate(0.5 + 0.5 * cos(6.28318530718 * (t + float3(0.0, -0.33, -0.67))));
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> destination = ResourceDescriptorHeap[bindings.chromatic_aberration_color_];
	ConstantBuffer<ChromaticAberrationConstantBuffer> constants = ResourceDescriptorHeap[bindings.chromatic_aberration_constants_];

	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	Texture2D<float4> source = ResourceDescriptorHeap[bindings.lens_distortion_color_];

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);

	float intensity = constants.intensity_;
	uint sample_count = clamp(constants.sample_count_, 3u, 16u);

	float2 offset_from_center = uv - 0.5;
	float2 maximum_offset = offset_from_center * intensity * 2.0;

	float3 accumulated_color = float3(0.0, 0.0, 0.0);
	float3 accumulated_weight = float3(0.0, 0.0, 0.0);

	[loop]
	for (uint index = 0; index < sample_count; index++)
	{
		float t = float(index) / float(sample_count - 1);
		float3 weight = SpectralResponse(t);

		float2 sample_uv = uv + maximum_offset * t;

		accumulated_color += source.SampleLevel(sampler_linear_clamp, sample_uv, 0).rgb * weight;
		accumulated_weight += weight;
	}

	destination[dtid.xy] = float4(accumulated_color / max(accumulated_weight, 0.0001), 1.0);
}
