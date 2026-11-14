#include <GraphicsEngine/Shader/Scene.hlsli>

struct PostProcessComposeConstantBuffer
{
	float exposure_compensation_;
	uint auto_exposure_;
	float bloom_intensity_;
	float post_process_compose_padding_0_;
};

struct PostProcessComposeBindings
{
	uint vignette_color_;
	uint kawase_bloom_radiance_;
	uint lens_flare_radiance_;
	uint anamorphic_flare_radiance_;
	uint auto_exposure_exposure_;
	uint post_process_constants_;
	uint post_process_color_;
};
ConstantBuffer<PostProcessComposeBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Compose(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<float4> output = ResourceDescriptorHeap[bindings.post_process_color_];
	uint width;
	uint height;
	output.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	ConstantBuffer<PostProcessComposeConstantBuffer> constants = ResourceDescriptorHeap[bindings.post_process_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[bindings.vignette_color_];
	Texture2D<float4> bloom = ResourceDescriptorHeap[bindings.kawase_bloom_radiance_];
	Texture2D<float4> lens_flare = ResourceDescriptorHeap[bindings.lens_flare_radiance_];
	Texture2D<float4> anamorphic_flare = ResourceDescriptorHeap[bindings.anamorphic_flare_radiance_];

	float2 uv = (float2(id.xy) + 0.5) / float2(width, height);
	float3 color = source.Load(int3(id.xy, 0)).rgb;
	color += lens_flare.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	color += bloom.SampleLevel(sampler_linear_clamp, uv, 0).rgb * constants.bloom_intensity_;
	color += anamorphic_flare.SampleLevel(sampler_linear_clamp, uv, 0).rgb;

	float exposure_ev = constants.exposure_compensation_;
	if (constants.auto_exposure_ != 0)
	{
		StructuredBuffer<float> exposure = ResourceDescriptorHeap[bindings.auto_exposure_exposure_];
		exposure_ev += exposure[0];
	}

	output[id.xy] = float4(color * exp2(exposure_ev), 1.0);
}
