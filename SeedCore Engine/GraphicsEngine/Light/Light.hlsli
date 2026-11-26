#ifndef __LIGHT_HLSL__
#define __LIGHT_HLSL__

#include <GraphicsEngine/Model/Model.hlsli>

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_RECT 2

struct LightStructuredBuffer
{
	float3 position_;
	float range_;

	float3 color_;
	float intensity_;

	float3 direction_;
	uint type_;

	float3 right_;
	float half_width_;

	float3 up_;
	float half_height_;

	float cos_half_angle_;
	float softness_;
	uint shadow_index_;
	uint light_structured_buffer_padding_0_;
};

struct LightConstantBuffer
{
	float3 sun_direction_;
	float sun_intensity_;

	float3 sun_color_;
	float sun_angular_radius_;

	float3 moon_color_;
	float moon_intensity_;

	float moon_angular_radius_;
	float moon_phase_;
	float night_factor_;
	uint light_count_;

	float sky_intensity_;
	uint3 light_constant_padding_0_;
};

#endif
