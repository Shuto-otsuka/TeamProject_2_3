#ifndef __TONE_MAPPING_HLSL__
#define __TONE_MAPPING_HLSL__

float3 ReinhardToneMap(float3 color)
{
	return color / (1.0 + color);
}

float3 AcesFilmicToneMap(float3 color)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

float3 PbrNeutralToneMap(float3 color)
{
	const float start_compression = 0.8 - 0.04;
	const float desaturation = 0.15;

	float min_channel = min(color.r, min(color.g, color.b));
	float offset = min_channel < 0.08 ? min_channel - 6.25 * min_channel * min_channel : 0.04;
	color -= offset;

	float peak = max(color.r, max(color.g, color.b));
	if (peak < start_compression)
	{
		return color;
	}

	float d = 1.0 - start_compression;
	float new_peak = 1.0 - d * d / (peak + d - start_compression);
	color *= new_peak / peak;

	float desaturation_amount = 1.0 - 1.0 / (desaturation * (peak - new_peak) + 1.0);
	return lerp(color, float3(new_peak, new_peak, new_peak), desaturation_amount);
}

float3 LinearToSrgb(float3 linear_color)
{
	float3 lower_range = linear_color * 12.92;
	float3 higher_range = 1.055 * pow(max(linear_color, 0.0), 1.0 / 2.4) - 0.055;
	return lerp(lower_range, higher_range, step(0.0031308, linear_color));
}

#endif
