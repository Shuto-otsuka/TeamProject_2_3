#ifndef __LIGHT_HLSL__
#define __LIGHT_HLSL__

#include <GraphicsEngine/Model/Model.hlsli>

// Mirrors the C++ LightType enum (Light/Light.h). Read from LightStructuredBuffer::type_.
#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_RECT 2

/**
* [EN]
* One entry of LightList, shared by point, spot and rectangle lights and told
* apart by type_. direction_ is the spot axis or the rectangle's facing
* normal; right_/up_ with half_width_/half_height_ span the rectangle;
* cos_half_angle_ and softness_ shape the spot cone. shadow_index_ is the
* light's shadow atlas region, MODEL_INVALID when it casts no atlas shadow.
*/
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

/**
* [EN]
* LightConstants: the sun and moon plus the light count and night factor.
* The moon's light travels exactly opposite sun_direction_ (the celestial
* system keeps the moon at -sun direction), so there is only one direction.
* The moon fields and night_factor_ are set only while the celestial cycle
* drives the frame; zero otherwise. moon_phase_/moon_angular_radius_ are
* sky-disc rendering parameters, not lighting terms. sky_intensity_ is the
* sky light's intensity that scales the image-based lighting; zero without
* an active SkyLight.
*/
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

#endif // __LIGHT_HLSL__
