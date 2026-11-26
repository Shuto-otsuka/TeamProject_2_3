#ifndef __WEATHER_HLSL__
#define __WEATHER_HLSL__

#include <GraphicsEngine/Shader/Scene.hlsli>

struct WeatherConstantBuffer
{
	float wetness_;

	float snow_coverage_;

	float thunder_flash_;

	float thunder_seed_;

	float snow_intensity_;

	float3 weather_constant_buffer_padding_0_;
};

struct WeatherParticleStructuredBuffer
{
	float3 position_;

	float life_;

	float3 velocity_;

	float seed_;
};

struct WeatherEmitterConstantBuffer
{
	float3 wind_;
	uint rain_capacity_;

	uint rain_count_;
	float rain_fall_speed_;
	float rain_size_;
	float rain_streak_length_;

	float rain_brightness_;
	float rain_volume_radius_;
	float rain_volume_height_;
	uint snow_count_;

	float3 rain_color_;
	float snow_fall_speed_;

	float snow_sway_amount_;
	float snow_size_;
	float snow_brightness_;
	float snow_volume_radius_;

	float3 snow_color_;
	float snow_volume_height_;
};

struct WeatherDrawBindings
{
	uint weather_particles_;
	uint weather_emitter_;
	uint raytracing_volumetric_light_fog_;
};
ConstantBuffer<WeatherDrawBindings> draw_bindings : register(b0, space1);

struct WeatherOutput
{
	float4 position : SV_Position;
	float2 local : TEXCOORD0;
	float3 color : COLOR0;
	nointerpolation uint rain : BLENDINDICES0;
};

#endif
