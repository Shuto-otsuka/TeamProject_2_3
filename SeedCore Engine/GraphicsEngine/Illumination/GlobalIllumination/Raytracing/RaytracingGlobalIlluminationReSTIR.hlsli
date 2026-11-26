#ifndef __GLOBAL_ILLUMINATION_RESTIR_HLSL__
#define __GLOBAL_ILLUMINATION_RESTIR_HLSL__

#include "../../../Shader/Noise.hlsli"

static const float GI_RESERVOIR_M_CAP = 8.0;

static const float GI_RESERVOIR_MAX_AGE = 30.0;

static const float GI_RESERVOIR_DEPTH_THRESHOLD = 0.1;

static const float GI_RESERVOIR_NORMAL_THRESHOLD = 0.5;

struct RaytracingGlobalIlluminationReservoir
{
	float3 sample_position_;

	float sample_m_;

	float3 sample_normal_;

	float sample_w_;

	float3 sample_radiance_;

	float sample_age_;

	float3 receiver_normal_;

	float receiver_depth_;
};

RaytracingGlobalIlluminationReservoir GlobalIlluminationReservoirFromSample(float3 position, float3 normal, float3 radiance)
{
	RaytracingGlobalIlluminationReservoir reservoir;
	reservoir.sample_position_ = position;
	reservoir.sample_normal_ = normal;
	reservoir.sample_radiance_ = radiance;
	reservoir.sample_m_ = 1.0;
	reservoir.sample_w_ = 1.0;
	reservoir.sample_age_ = 0.0;
	reservoir.receiver_normal_ = float3(0, 0, 0);
	reservoir.receiver_depth_ = 0.0;
	return reservoir;
}

RaytracingGlobalIlluminationReservoir GlobalIlluminationReservoirCombine(RaytracingGlobalIlluminationReservoir a, RaytracingGlobalIlluminationReservoir b, bool b_valid, inout uint rng_state)
{
	if (!b_valid)
	{
		return a;
	}

	RaytracingGlobalIlluminationReservoir result = a;

	float a_target = dot(a.sample_radiance_, float3(0.2126, 0.7152, 0.0722));
	float a_weight = a_target * a.sample_w_ * a.sample_m_;

	float b_target = dot(b.sample_radiance_, float3(0.2126, 0.7152, 0.0722));
	float b_weight = b_target * b.sample_w_ * b.sample_m_;

	float weight_sum = a_weight + b_weight;
	result.sample_m_ = a.sample_m_ + b.sample_m_;

	if (b_weight > 0.0 && Rand(rng_state) < b_weight / weight_sum)
	{
		result.sample_position_ = b.sample_position_;
		result.sample_normal_ = b.sample_normal_;
		result.sample_radiance_ = b.sample_radiance_;
		result.sample_age_ = b.sample_age_;
	}

	float selected_target = dot(result.sample_radiance_, float3(0.2126, 0.7152, 0.0722));
	result.sample_w_ = selected_target > 0.0 ? (weight_sum / (result.sample_m_ * selected_target)) : 0.0;

	return result;
}

#endif
