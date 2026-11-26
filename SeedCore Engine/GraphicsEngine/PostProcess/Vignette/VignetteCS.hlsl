#include <GraphicsEngine/Shader/Scene.hlsli>

struct VignetteConstantBuffer
{
	float intensity_;
	float exponent_;
	float2 vignette_padding_0_;
	float4 color_;
};

struct VignetteResolveBindings
{
	uint chromatic_aberration_color_;
	uint vignette_constants_;
	uint vignette_color_;
};
ConstantBuffer<VignetteResolveBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> destination = ResourceDescriptorHeap[bindings.vignette_color_];
	ConstantBuffer<VignetteConstantBuffer> constants = ResourceDescriptorHeap[bindings.vignette_constants_];

	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	Texture2D<float4> source = ResourceDescriptorHeap[bindings.chromatic_aberration_color_];

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);
	float3 color = source.SampleLevel(sampler_linear_clamp, uv, 0).rgb;

	float intensity = constants.intensity_;
	float exponent = constants.exponent_;
	float3 vignette_color = constants.color_.rgb;

	float aspect = float(width) / float(height);
	float2 centered = (uv - 0.5) * float2(aspect, 1.0);
	float radius = length(centered) / length(float2(0.5 * aspect, 0.5));

	float angle = atan(radius);
	float falloff = pow(saturate(cos(angle)), max(exponent, 0.0));

	float blend = pow(saturate(falloff), max(intensity, 0.0));

	destination[dtid.xy] = float4(lerp(vignette_color, color, blend), 1.0);
}
