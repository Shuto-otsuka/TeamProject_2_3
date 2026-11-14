#ifndef __WEATHER_HLSL__
#define __WEATHER_HLSL__

#include <GraphicsEngine/Shader/Scene.hlsli>

/**
* [EN]
* Weather.Constants: the scene's weather state. Zero when the scene has no
* Weather component.
*/
struct WeatherConstantBuffer
{
	/// 0..1, wet-surface shading (rain plus a drying-out tail).
	float wetness_;

	/// 0..1, snow accumulation shading (builds up, melts over time).
	float snow_coverage_;

	/// 0..1, brief sky and ambient brightness burst on a lightning strike.
	float thunder_flash_;

	/// Re-rolled each strike; randomizes the lightning bolt shape.
	float thunder_seed_;

	/// 0..1, whether it is snowing right now (fast ramp, unlike snow_coverage_).
	float snow_intensity_;

	float3 weather_constant_buffer_padding_0_;
};

/**
* [EN]
* One rain drop or snow flake of Weather.Particles. The buffer holds the rain
* pool first (rain_capacity_ entries) and the snow pool after it.
*/
struct WeatherParticleStructuredBuffer
{
	float3 position_;

	/// 0..1, fade-in progress since the last respawn.
	float life_;

	float3 velocity_;

	/// Per-particle random, used for size, sway and respawn variance.
	float seed_;
};

/**
* [EN]
* Weather.Emitter: how many of each pool fall this frame and how they look.
* rain_count_/snow_count_ are the active prefixes of the two pools; the
* volume radius and height describe the camera-relative cylinder particles
* live in, spawning at its top.
*/
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

/**
* [EN]
* Per-vertex output of WeatherMS/WeatherVS: the corner of the quad in -1..1,
* the particle's color already scaled by brightness and fade-in, and whether
* it is rain (a streak) or snow (a round flake).
*/
struct WeatherOutput
{
	float4 position : SV_Position;
	float2 local : TEXCOORD0;
	float3 color : COLOR0;
	nointerpolation uint rain : BLENDINDICES0;
};

#endif // __WEATHER_HLSL__
