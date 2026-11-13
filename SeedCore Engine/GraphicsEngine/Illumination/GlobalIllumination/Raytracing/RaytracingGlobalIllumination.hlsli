#ifndef __RAYTRACING_GLOBAL_ILLUMINATION_HLSLI__
#define __RAYTRACING_GLOBAL_ILLUMINATION_HLSLI__

struct RaytracingGlobalIlluminationConstantBuffer
{
	float ray_t_max_;
	float normal_bias_;
	float intensity_;
	uint ray_count_;

	uint bounce_count_;
	uint frame_;
	uint temporal_reuse_;
	uint history_valid_;
};

#endif // __RAYTRACING_GLOBAL_ILLUMINATION_HLSLI__
