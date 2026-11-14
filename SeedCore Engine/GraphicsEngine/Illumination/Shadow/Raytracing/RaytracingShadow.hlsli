#ifndef __RAYTRACING_SHADOW_HLSLI__
#define __RAYTRACING_SHADOW_HLSLI__

struct RaytracingShadowConstantBuffer
{
	float ray_t_max_;
	float normal_bias_;
	float strength_;
	float sun_angular_radius_;

	float punctual_radius_;
	uint ray_count_;
	uint frame_;
	uint history_valid_;
};

#endif // __RAYTRACING_SHADOW_HLSLI__
