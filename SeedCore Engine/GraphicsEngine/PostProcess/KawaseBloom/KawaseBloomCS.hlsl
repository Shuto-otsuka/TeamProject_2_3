#include <GraphicsEngine/Shader/Scene.hlsli>

struct KawaseBloomConstantBuffer
{
	float threshold_;
	float soft_knee_;
	float filter_radius_;
	float kawase_bloom_padding_0_;
};

float Luminance(float3 color)
{
	return dot(color, float3(0.2126, 0.7152, 0.0722));
}

float KarisWeight(float3 color)
{
	return 1.0 / (1.0 + Luminance(color));
}

float3 SanitizeSceneColor(float3 color)
{
	bool invalid = any(isnan(color)) || any(isinf(color));
	return invalid ? float3(0, 0, 0) : max(color, 0.0);
}

float3 SoftThreshold(float3 color, float threshold, float soft_knee)
{
	float knee = max(threshold * soft_knee, 1e-4);
	float brightest = max(color.r, max(color.g, color.b));

	float soft = clamp(brightest - threshold + knee, 0.0, 2.0 * knee);
	soft = soft * soft / (4.0 * knee);

	float contribution = max(soft, brightest - threshold) / max(brightest, 1e-4);
	return color * contribution;
}

float3 Downsample13Tap(Texture2D<float4> source, float2 uv, float2 texel)
{
	float3 a = source.SampleLevel(sampler_linear_clamp, uv + float2(-2.0, 2.0) * texel, 0).rgb;
	float3 b = source.SampleLevel(sampler_linear_clamp, uv + float2(0.0, 2.0) * texel, 0).rgb;
	float3 c = source.SampleLevel(sampler_linear_clamp, uv + float2(2.0, 2.0) * texel, 0).rgb;

	float3 d = source.SampleLevel(sampler_linear_clamp, uv + float2(-2.0, 0.0) * texel, 0).rgb;
	float3 e = source.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	float3 f = source.SampleLevel(sampler_linear_clamp, uv + float2(2.0, 0.0) * texel, 0).rgb;

	float3 g = source.SampleLevel(sampler_linear_clamp, uv + float2(-2.0, -2.0) * texel, 0).rgb;
	float3 h = source.SampleLevel(sampler_linear_clamp, uv + float2(0.0, -2.0) * texel, 0).rgb;
	float3 i = source.SampleLevel(sampler_linear_clamp, uv + float2(2.0, -2.0) * texel, 0).rgb;

	float3 j = source.SampleLevel(sampler_linear_clamp, uv + float2(-1.0, 1.0) * texel, 0).rgb;
	float3 k = source.SampleLevel(sampler_linear_clamp, uv + float2(1.0, 1.0) * texel, 0).rgb;
	float3 l = source.SampleLevel(sampler_linear_clamp, uv + float2(-1.0, -1.0) * texel, 0).rgb;
	float3 m = source.SampleLevel(sampler_linear_clamp, uv + float2(1.0, -1.0) * texel, 0).rgb;

	float3 result = e * 0.125;
	result += (a + c + g + i) * 0.03125;
	result += (b + d + f + h) * 0.0625;
	result += (j + k + l + m) * 0.125;
	return result;
}

float3 Downsample13TapKaris(Texture2D<float4> source, float2 uv, float2 texel)
{
	float3 a = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(-2.0, 2.0) * texel, 0).rgb);
	float3 b = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(0.0, 2.0) * texel, 0).rgb);
	float3 c = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(2.0, 2.0) * texel, 0).rgb);

	float3 d = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(-2.0, 0.0) * texel, 0).rgb);
	float3 e = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv, 0).rgb);
	float3 f = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(2.0, 0.0) * texel, 0).rgb);

	float3 g = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(-2.0, -2.0) * texel, 0).rgb);
	float3 h = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(0.0, -2.0) * texel, 0).rgb);
	float3 i = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(2.0, -2.0) * texel, 0).rgb);

	float3 j = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(-1.0, 1.0) * texel, 0).rgb);
	float3 k = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(1.0, 1.0) * texel, 0).rgb);
	float3 l = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(-1.0, -1.0) * texel, 0).rgb);
	float3 m = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(1.0, -1.0) * texel, 0).rgb);

	float3 group0 = (a + b + d + e) * 0.25;
	float3 group1 = (b + c + e + f) * 0.25;
	float3 group2 = (d + e + g + h) * 0.25;
	float3 group3 = (e + f + h + i) * 0.25;
	float3 group4 = (j + k + l + m) * 0.25;

	float weight0 = KarisWeight(group0) * 0.125;
	float weight1 = KarisWeight(group1) * 0.125;
	float weight2 = KarisWeight(group2) * 0.125;
	float weight3 = KarisWeight(group3) * 0.125;
	float weight4 = KarisWeight(group4) * 0.5;

	float weight_sum = weight0 + weight1 + weight2 + weight3 + weight4;
	float3 result = group0 * weight0 + group1 * weight1 + group2 * weight2 + group3 * weight3 + group4 * weight4;
	return result / max(weight_sum, 1e-4);
}

float3 UpsampleTent(Texture2D<float4> source, float2 uv, float radius)
{
	float3 a = source.SampleLevel(sampler_linear_clamp, uv + float2(-radius, radius), 0).rgb;
	float3 b = source.SampleLevel(sampler_linear_clamp, uv + float2(0.0, radius), 0).rgb;
	float3 c = source.SampleLevel(sampler_linear_clamp, uv + float2(radius, radius), 0).rgb;

	float3 d = source.SampleLevel(sampler_linear_clamp, uv + float2(-radius, 0.0), 0).rgb;
	float3 e = source.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	float3 f = source.SampleLevel(sampler_linear_clamp, uv + float2(radius, 0.0), 0).rgb;

	float3 g = source.SampleLevel(sampler_linear_clamp, uv + float2(-radius, -radius), 0).rgb;
	float3 h = source.SampleLevel(sampler_linear_clamp, uv + float2(0.0, -radius), 0).rgb;
	float3 i = source.SampleLevel(sampler_linear_clamp, uv + float2(radius, -radius), 0).rgb;

	float3 result = e * 4.0;
	result += (b + d + f + h) * 2.0;
	result += (a + c + g + i);
	return result * (1.0 / 16.0);
}

void DownsampleLevel(uint3 dtid, Texture2D<float4> source, RWTexture2D<float4> destination)
{
	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	uint source_width, source_height;
	source.GetDimensions(source_width, source_height);
	float2 source_texel = 1.0 / float2(source_width, source_height);

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);

	destination[dtid.xy] = float4(Downsample13Tap(source, uv, source_texel), 1.0);
}

void UpsampleLevel(uint3 dtid, Texture2D<float4> source, RWTexture2D<float4> destination, float radius)
{
	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);

	float3 result = destination[dtid.xy].rgb + UpsampleTent(source, uv, radius);
	destination[dtid.xy] = float4(result, 1.0);
}

struct KawaseBloomPrefilterBindings
{
	uint bokeh_color_;
	uint kawase_bloom_constants_;
	uint kawase_bloom_radiance_;
};
ConstantBuffer<KawaseBloomPrefilterBindings> prefilter_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Prefilter(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> destination = ResourceDescriptorHeap[prefilter_bindings.kawase_bloom_radiance_];

	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	ConstantBuffer<KawaseBloomConstantBuffer> constants = ResourceDescriptorHeap[prefilter_bindings.kawase_bloom_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[prefilter_bindings.bokeh_color_];

	uint source_width, source_height;
	source.GetDimensions(source_width, source_height);
	float2 source_texel = 1.0 / float2(source_width, source_height);

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);

	float3 result = Downsample13TapKaris(source, uv, source_texel);
	result = SoftThreshold(result, constants.threshold_, constants.soft_knee_);

	destination[dtid.xy] = float4(result, 1.0);
}

struct KawaseBloomDownsampleBindings
{
	uint kawase_bloom_source_;
	uint kawase_bloom_destination_;
};
ConstantBuffer<KawaseBloomDownsampleBindings> downsample_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Downsample(uint3 dtid : SV_DispatchThreadID)
{
	Texture2D<float4> source = ResourceDescriptorHeap[downsample_bindings.kawase_bloom_source_];
	RWTexture2D<float4> destination = ResourceDescriptorHeap[downsample_bindings.kawase_bloom_destination_];
	DownsampleLevel(dtid, source, destination);
}

struct KawaseBloomUpsampleBindings
{
	uint kawase_bloom_constants_;
	uint kawase_bloom_source_;
	uint kawase_bloom_destination_;
};
ConstantBuffer<KawaseBloomUpsampleBindings> upsample_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Upsample(uint3 dtid : SV_DispatchThreadID)
{
	ConstantBuffer<KawaseBloomConstantBuffer> constants = ResourceDescriptorHeap[upsample_bindings.kawase_bloom_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[upsample_bindings.kawase_bloom_source_];
	RWTexture2D<float4> destination = ResourceDescriptorHeap[upsample_bindings.kawase_bloom_destination_];
	UpsampleLevel(dtid, source, destination, constants.filter_radius_);
}
