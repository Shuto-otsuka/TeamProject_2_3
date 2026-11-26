#ifndef __DENOISER_HLSL__
#define __DENOISER_HLSL__

#include "Sampler.hlsli"
#include "Normal.hlsli"

float DenoiserSpatialWeight(float center_depth, float2 depth_gradient, int2 offset, float neighbor_depth, float3 center_normal, float3 neighbor_normal, float depth_sharpness, float normal_power)
{
	float expected_depth = center_depth + dot(depth_gradient, float2(offset));
	float depth_difference = abs(neighbor_depth - expected_depth) / max(center_depth, 0.0001);
	float depth_weight = exp(-depth_difference * depth_sharpness);

	float normal_weight = pow(saturate(dot(center_normal, neighbor_normal)), normal_power);

	return depth_weight * normal_weight;
}

float2 DenoiserDepthGradient(Texture2D<float> depth_texture, int2 pixel, int2 screen_max)
{
	float depth_right = depth_texture.Load(int3(clamp(pixel + int2(1, 0), int2(0, 0), screen_max), 0));
	float depth_left = depth_texture.Load(int3(clamp(pixel + int2(-1, 0), int2(0, 0), screen_max), 0));
	float depth_down = depth_texture.Load(int3(clamp(pixel + int2(0, 1), int2(0, 0), screen_max), 0));
	float depth_up = depth_texture.Load(int3(clamp(pixel + int2(0, -1), int2(0, 0), screen_max), 0));
	return float2(depth_right - depth_left, depth_down - depth_up) * 0.5;
}

struct DenoiserMoments
{
	float weight_sum_;

	float3 moment1_sum_;

	float3 moment2_sum_;

	float3 neighborhood_min_;
	float3 neighborhood_max_;
};

DenoiserMoments DenoiserMomentsInit()
{
	DenoiserMoments moments;
	moments.weight_sum_ = 0.0;
	moments.moment1_sum_ = float3(0, 0, 0);
	moments.moment2_sum_ = float3(0, 0, 0);
	moments.neighborhood_min_ = float3(1e5, 1e5, 1e5);
	moments.neighborhood_max_ = float3(0, 0, 0);
	return moments;
}

void DenoiserMomentsAccumulate(inout DenoiserMoments moments, float3 sample_value, float weight)
{
	moments.moment1_sum_ += sample_value * weight;
	moments.moment2_sum_ += sample_value * sample_value * weight;
	moments.weight_sum_ += weight;
	moments.neighborhood_min_ = min(moments.neighborhood_min_, sample_value);
	moments.neighborhood_max_ = max(moments.neighborhood_max_, sample_value);
}

float3 DenoiserVarianceClipMin(DenoiserMoments moments, float3 filtered_raw, float clip_gamma)
{
	float3 mean = moments.weight_sum_ > 0.0001 ? moments.moment1_sum_ / moments.weight_sum_ : filtered_raw;
	float3 mean_of_squares = moments.weight_sum_ > 0.0001 ? moments.moment2_sum_ / moments.weight_sum_ : filtered_raw * filtered_raw;
	float3 variance = max(mean_of_squares - mean * mean, 0.0);
	float3 clip_radius = clip_gamma * sqrt(variance);
	float3 band_min = min(moments.neighborhood_min_, filtered_raw);
	return max(mean - clip_radius, band_min);
}

float3 DenoiserVarianceClipMax(DenoiserMoments moments, float3 filtered_raw, float clip_gamma)
{
	float3 mean = moments.weight_sum_ > 0.0001 ? moments.moment1_sum_ / moments.weight_sum_ : filtered_raw;
	float3 mean_of_squares = moments.weight_sum_ > 0.0001 ? moments.moment2_sum_ / moments.weight_sum_ : filtered_raw * filtered_raw;
	float3 variance = max(mean_of_squares - mean * mean, 0.0);
	float3 clip_radius = clip_gamma * sqrt(variance);
	float3 band_max = max(moments.neighborhood_max_, filtered_raw);
	return min(mean + clip_radius, band_max);
}

float3 DenoiserTemporalBlend(Texture2D<float4> history_texture, float2 previous_uv, float3 clip_min, float3 clip_max, float3 filtered_raw, float blend_alpha)
{
	if (previous_uv.x < 0.0 || previous_uv.x > 1.0 || previous_uv.y < 0.0 || previous_uv.y > 1.0)
	{
		return filtered_raw;
	}

	float4 history_sample = history_texture.SampleLevel(sampler_linear_clamp, previous_uv, 0);
	if (history_sample.a <= 0.5)
	{
		return filtered_raw;
	}

	float3 history_value = clamp(history_sample.rgb, clip_min, clip_max);
	return lerp(history_value, filtered_raw, blend_alpha);
}

float DenoiserLuminance(float3 color)
{
	return dot(color, float3(0.2126, 0.7152, 0.0722));
}

static const float DENOISER_FP16_MAX = 65504.0;

float3 DenoiserSanitizeRadiance(float3 radiance)
{
	bool invalid = any(isnan(radiance)) || any(isinf(radiance));
	return invalid ? float3(0, 0, 0) : clamp(radiance, 0.0, DENOISER_FP16_MAX);
}

static const float DENOISER_ATROUS_KERNEL[5] = { 0.0625, 0.25, 0.375, 0.25, 0.0625 };

float3 DenoiserATrousPass(Texture2D<float4> source_texture, Texture2D<float> depth_texture, Texture2D<float4> normal_texture, int2 pixel, int2 screen_max, float center_depth, float2 depth_gradient, float3 center_normal, int step, float depth_sharpness, float normal_power)
{
	float3 sum = float3(0, 0, 0);
	float weight_sum = 0.0;

	[unroll]
	for (int ty = -2; ty <= 2; ++ty)
	{
		[unroll]
		for (int tx = -2; tx <= 2; ++tx)
		{
			int2 offset = int2(tx, ty) * step;
			int2 neighbor = clamp(pixel + offset, int2(0, 0), screen_max);

			float4 neighbor_sample = source_texture.Load(int3(neighbor, 0));
			float neighbor_depth = depth_texture.Load(int3(neighbor, 0));
			float3 neighbor_normal = OctNormalDecode(normal_texture.Load(int3(neighbor, 0)).rg);

			float kernel_weight = DENOISER_ATROUS_KERNEL[tx + 2] * DENOISER_ATROUS_KERNEL[ty + 2];
			float edge_weight = DenoiserSpatialWeight(center_depth, depth_gradient, offset, neighbor_depth, center_normal, neighbor_normal, depth_sharpness, normal_power);

			float weight = kernel_weight * edge_weight * neighbor_sample.a;
			sum += neighbor_sample.rgb * weight;
			weight_sum += weight;
		}
	}

	return weight_sum > 0.0001 ? sum / weight_sum : source_texture.Load(int3(pixel, 0)).rgb;
}

float3 DenoiserViewPosition(float4x4 inverse_projection, float2 uv, float depth)
{
	float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
	float4 view_position = mul(float4(ndc, depth, 1.0), inverse_projection);
	return view_position.xyz / view_position.w;
}

float3 DenoiserWorldPosition(float4x4 inverse_view_projection, float2 uv, float depth)
{
	float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
	float4 world_position = mul(float4(ndc, depth, 1.0), inverse_view_projection);
	return world_position.xyz / world_position.w;
}

float2 DenoiserPreviousUv(float2 uv, float2 velocity)
{
	return uv - float2(velocity.x, -velocity.y);
}

float4 SvgfPackDepthNormal(float view_z, float view_z_derivative, float3 normal)
{
	return float4(view_z, view_z_derivative, OctNormalEncode(normal));
}

float3 SvgfUnpackNormal(float4 packed_depth_normal)
{
	return OctNormalDecode(packed_depth_normal.zw);
}

float SvgfViewDepthDerivative(float left, float right, float up, float down)
{
	return max(abs(right - left), abs(down - up)) * 0.5;
}

float SvgfDepthNormalWeight(float center_view_z, float neighbor_view_z, float phi_depth, float3 center_normal, float3 neighbor_normal, float phi_normal)
{
	float normal_weight = pow(saturate(dot(center_normal, neighbor_normal)), phi_normal);
	float depth_weight = phi_depth <= 0.0 ? 0.0 : abs(center_view_z - neighbor_view_z) / phi_depth;

	return exp(-max(depth_weight, 0.0)) * normal_weight;
}

float2 SvgfLuminanceWeight(float2 center_luminance, float2 neighbor_luminance, float2 phi_luminance)
{
	return exp(-abs(center_luminance - neighbor_luminance) / phi_luminance);
}

float SvgfLuminanceWeight(float center_luminance, float neighbor_luminance, float phi_luminance)
{
	return exp(-abs(center_luminance - neighbor_luminance) / phi_luminance);
}

float2 SvgfVarianceCenter(Texture2D<float4> source_texture, int2 pixel, int2 screen_max)
{
	const float kernel[3] = { 1.0 / 4.0, 1.0 / 8.0, 1.0 / 16.0 };

	float2 sum = float2(0, 0);

	[unroll]
	for (int dy = -1; dy <= 1; ++dy)
	{
		[unroll]
		for (int dx = -1; dx <= 1; ++dx)
		{
			int2 neighbor = clamp(pixel + int2(dx, dy), int2(0, 0), screen_max);
			sum += source_texture.Load(int3(neighbor, 0)).ba * kernel[abs(dx) + abs(dy)];
		}
	}

	return sum;
}

float SvgfVarianceCenterScalar(Texture2D<float2> source_texture, int2 pixel, int2 screen_max)
{
	const float kernel[3] = { 1.0 / 4.0, 1.0 / 8.0, 1.0 / 16.0 };

	float sum = 0.0;

	[unroll]
	for (int dy = -1; dy <= 1; ++dy)
	{
		[unroll]
		for (int dx = -1; dx <= 1; ++dx)
		{
			int2 neighbor = clamp(pixel + int2(dx, dy), int2(0, 0), screen_max);
			sum += source_texture.Load(int3(neighbor, 0)).y * kernel[abs(dx) + abs(dy)];
		}
	}

	return sum;
}

float SvgfVarianceCenterLuminance(Texture2D<float4> source_texture, int2 pixel, int2 screen_max)
{
	const float kernel[3] = { 1.0 / 4.0, 1.0 / 8.0, 1.0 / 16.0 };

	float sum = 0.0;

	[unroll]
	for (int dy = -1; dy <= 1; ++dy)
	{
		[unroll]
		for (int dx = -1; dx <= 1; ++dx)
		{
			int2 neighbor = clamp(pixel + int2(dx, dy), int2(0, 0), screen_max);
			sum += source_texture.Load(int3(neighbor, 0)).a * kernel[abs(dx) + abs(dy)];
		}
	}

	return sum;
}

static const float SVGF_ATROUS_KERNEL[3] = { 1.0, 2.0 / 3.0, 1.0 / 6.0 };

#endif
