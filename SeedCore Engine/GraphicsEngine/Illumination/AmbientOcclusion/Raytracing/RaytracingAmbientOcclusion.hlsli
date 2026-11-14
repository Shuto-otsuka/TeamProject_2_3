#ifndef __RAYTRACING_AMBIENT_OCCLUSION_HLSLI__
#define __RAYTRACING_AMBIENT_OCCLUSION_HLSLI__

struct RaytracingAmbientOcclusionConstantBuffer
{
	float ray_length_;
	float normal_bias_;
	float power_;
	uint ray_count_;

	uint frame_;
	uint history_valid_;
	uint raytracing_ambient_occlusion_constant_buffer_padding_0_;
	uint raytracing_ambient_occlusion_constant_buffer_padding_1_;
};

#endif // __RAYTRACING_AMBIENT_OCCLUSION_HLSLI__
