#include <GraphicsEngine/Shader/Scene.hlsli>

struct AnamorphicFlareConstantBuffer
{
	float threshold_;
	float intensity_;
	float streak_length_;
	float attenuation_;
	float4 tint_;
};

float3 AnamorphicBrightPass(float3 color, float threshold)
{
	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	float weight = max(luminance - threshold, 0.0) / max(luminance, 0.0001);
	return color * weight;
}

float3 AnamorphicStreakGather(Texture2D<float4> source, float2 uv, float texel_width, float spacing, float attenuation, float length_scale)
{
	float safe_attenuation = clamp(attenuation, 0.5, 0.999);

	float3 sum = source.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	float weight_sum = 1.0;

	[unroll]
	for (uint tap = 1; tap <= 3; tap++)
	{
		float texel_distance = spacing * float(tap);
		float weight = pow(safe_attenuation, texel_distance);
		float offset = texel_width * texel_distance * length_scale;

		sum += source.SampleLevel(sampler_linear_clamp, uv + float2(offset, 0.0), 0).rgb * weight;
		sum += source.SampleLevel(sampler_linear_clamp, uv - float2(offset, 0.0), 0).rgb * weight;
		weight_sum += weight * 2.0;
	}
	return sum / weight_sum;
}

void AnamorphicBlurPass(uint3 dtid, float spacing, Texture2D<float4> source, RWTexture2D<float4> destination, ConstantBuffer<AnamorphicFlareConstantBuffer> constants)
{
	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);
	float texel_width = 1.0 / float(width);
	float attenuation = constants.attenuation_;
	float length_scale = constants.streak_length_ * 4.0;

	destination[dtid.xy] = float4(AnamorphicStreakGather(source, uv, texel_width, spacing, attenuation, length_scale), 1.0);
}

struct AnamorphicFlarePrefilterBindings
{
	uint bokeh_color_;
	uint anamorphic_flare_constants_;
	uint anamorphic_flare_ping_;
};
ConstantBuffer<AnamorphicFlarePrefilterBindings> prefilter_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Prefilter(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> destination = ResourceDescriptorHeap[prefilter_bindings.anamorphic_flare_ping_];

	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	ConstantBuffer<AnamorphicFlareConstantBuffer> constants = ResourceDescriptorHeap[prefilter_bindings.anamorphic_flare_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[prefilter_bindings.bokeh_color_];
	uint source_width, source_height;
	source.GetDimensions(source_width, source_height);
	float2 source_texel = 1.0 / float2(source_width, source_height);

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);
	float threshold = constants.threshold_;

	float3 result = float3(0.0, 0.0, 0.0);

	[unroll]
	for (uint row = 0; row < 2; row++)
	{
		float vertical_offset = (float(row) * 2.0 - 1.0) * source_texel.y;

		[unroll]
		for (uint column = 0; column < 4; column++)
		{
			float horizontal_offset = (float(column) * 2.0 - 3.0) * source_texel.x;
			float2 tap_uv = uv + float2(horizontal_offset, vertical_offset);

			result += AnamorphicBrightPass(source.SampleLevel(sampler_linear_clamp, tap_uv, 0).rgb, threshold);
		}
	}

	destination[dtid.xy] = float4(result * 0.125, 1.0);
}

struct AnamorphicFlareBlurBindings
{
	uint anamorphic_flare_constants_;
	uint anamorphic_flare_source_;
	uint anamorphic_flare_destination_;
};
ConstantBuffer<AnamorphicFlareBlurBindings> blur_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Blur(uint3 dtid : SV_DispatchThreadID)
{
	ConstantBuffer<AnamorphicFlareConstantBuffer> constants = ResourceDescriptorHeap[blur_bindings.anamorphic_flare_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[blur_bindings.anamorphic_flare_source_];
	RWTexture2D<float4> destination = ResourceDescriptorHeap[blur_bindings.anamorphic_flare_destination_];
	AnamorphicBlurPass(dtid, float(1u << (dispatch_bindings.constants_ * 2)), source, destination, constants);
}

struct AnamorphicFlareComposeBindings
{
	uint anamorphic_flare_constants_;
	uint anamorphic_flare_ping_;
	uint anamorphic_flare_radiance_;
};
ConstantBuffer<AnamorphicFlareComposeBindings> compose_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Compose(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> output = ResourceDescriptorHeap[compose_bindings.anamorphic_flare_radiance_];

	uint width, height;
	output.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	ConstantBuffer<AnamorphicFlareConstantBuffer> constants = ResourceDescriptorHeap[compose_bindings.anamorphic_flare_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[compose_bindings.anamorphic_flare_ping_];

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);
	float intensity = constants.intensity_;
	float3 tint = constants.tint_.rgb;

	float3 streak = source.SampleLevel(sampler_linear_clamp, uv, 0).rgb;

	output[dtid.xy] = float4(streak * tint * intensity, 1.0);
}
