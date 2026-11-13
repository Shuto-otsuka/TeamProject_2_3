#ifndef __RAYTRACING_REFLECTION_HLSLI__
#define __RAYTRACING_REFLECTION_HLSLI__

struct RaytracingReflectionConstantBuffer
{
	float ray_t_max_;
	float normal_bias_;
	float strength_;
	uint ray_count_;

	uint frame_;
	uint temporal_reuse_;
	uint history_valid_;
	uint raytracing_reflection_constant_buffer_padding_0_;
};

#endif // __RAYTRACING_REFLECTION_HLSLI__
