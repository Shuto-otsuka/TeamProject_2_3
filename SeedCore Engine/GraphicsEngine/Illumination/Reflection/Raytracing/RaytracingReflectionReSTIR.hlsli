#ifndef __REFLECTION_RESTIR_HLSL__
#define __REFLECTION_RESTIR_HLSL__

#include "../../../Shader/Noise.hlsli"

static const float REFLECTION_RESERVOIR_M_CAP = 8.0;

static const float REFLECTION_RESERVOIR_M_CAP_ROUGHNESS = 0.5;

static const float REFLECTION_RESERVOIR_MAX_AGE = 30.0;

static const float REFLECTION_RESERVOIR_DEPTH_THRESHOLD = 0.1;

static const float REFLECTION_RESERVOIR_NORMAL_THRESHOLD = 0.5;

struct RaytracingReflectionReservoir
{
	float3 sample_direction_;

	float sample_m_;

	float3 sample_radiance_;

	float sample_w_;

	float3 receiver_normal_;

	float receiver_depth_;

	float sample_hit_distance_;

	float sample_age_;

	float2 reflection_reservoir_padding_;
};

RaytracingReflectionReservoir ReflectionReservoirFromSample(float3 direction, float3 radiance, float hit_distance)
{
	RaytracingReflectionReservoir reservoir;
	reservoir.sample_direction_ = direction;
	reservoir.sample_radiance_ = radiance;
	reservoir.sample_hit_distance_ = hit_distance;
	reservoir.sample_m_ = 1.0;
	reservoir.sample_w_ = 1.0;
	reservoir.receiver_normal_ = float3(0, 0, 0);
	reservoir.receiver_depth_ = 0.0;
	reservoir.sample_age_ = 0.0;
	reservoir.reflection_reservoir_padding_ = float2(0, 0);
	return reservoir;
}

RaytracingReflectionReservoir ReflectionReservoirCombine(RaytracingReflectionReservoir a, RaytracingReflectionReservoir b, bool b_valid, inout uint rng_state)
{
	if (!b_valid)
	{
		return a;
	}

	RaytracingReflectionReservoir result = a;

	float a_target = dot(a.sample_radiance_, float3(0.2126, 0.7152, 0.0722));
	float a_weight = a_target * a.sample_w_ * a.sample_m_;

	float b_target = dot(b.sample_radiance_, float3(0.2126, 0.7152, 0.0722));
	float b_weight = b_target * b.sample_w_ * b.sample_m_;

	float weight_sum = a_weight + b_weight;
	result.sample_m_ = a.sample_m_ + b.sample_m_;

	if (b_weight > 0.0 && Rand(rng_state) < b_weight / weight_sum)
	{
		result.sample_direction_ = b.sample_direction_;
		result.sample_radiance_ = b.sample_radiance_;
		result.sample_hit_distance_ = b.sample_hit_distance_;
		result.sample_age_ = b.sample_age_;
	}

	float selected_target = dot(result.sample_radiance_, float3(0.2126, 0.7152, 0.0722));
	result.sample_w_ = selected_target > 0.0 ? (weight_sum / (result.sample_m_ * selected_target)) : 0.0;
	result.reflection_reservoir_padding_ = float2(0, 0);

	return result;
}

#endif
