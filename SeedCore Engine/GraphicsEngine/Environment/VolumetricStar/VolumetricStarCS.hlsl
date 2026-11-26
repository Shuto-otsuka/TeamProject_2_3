#include <GraphicsEngine/Environment/VolumetricStar/VolumetricStar.hlsli>
#include <GraphicsEngine/Light/Light.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>

struct VolumetricStarTraceBindings
{
	uint volumetric_star_constants_;
	uint geometry_depth_;
	uint light_constants_;
	uint volumetric_star_radiance_;
};
ConstantBuffer<VolumetricStarTraceBindings> trace_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Trace(uint3 dtid : SV_DispatchThreadID)
{
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	RWTexture2D<float4> output = ResourceDescriptorHeap[trace_bindings.volumetric_star_radiance_];

	uint star_width;
	uint star_height;
	output.GetDimensions(star_width, star_height);

	if (dtid.x >= star_width || dtid.y >= star_height)
	{
		return;
	}

	uint2 pixel = dtid.xy;

	Texture2D<float> depth_texture = ResourceDescriptorHeap[trace_bindings.geometry_depth_];
	if (depth_texture.Load(int3(pixel, 0)) != 0.0)
	{
		output[pixel] = float4(0, 0, 0, 0);
		return;
	}

	ConstantBuffer<VolumetricStarConstantBuffer> tuning = ResourceDescriptorHeap[trace_bindings.volumetric_star_constants_];

	if (tuning.enabled_ == 0)
	{
		output[pixel] = float4(0, 0, 0, 0);
		return;
	}

	float2 uv = (float2(pixel) + 0.5) / float2(star_width, star_height);
	float2 ndc = float2(uv.x * 2 - 1, 1 - uv.y * 2);
	float4 far_clip = float4(ndc, 0.0, 1.0);
	float4 far_world = mul(far_clip, scene.inverse_view_projection_);
	float3 view_direction = normalize(far_world.xyz / far_world.w - scene.camera_position_.xyz);

	ConstantBuffer<LightConstantBuffer> light = ResourceDescriptorHeap[trace_bindings.light_constants_];
	float night_factor = light.night_factor_;

	float3 color = StarFieldColor(view_direction, night_factor, scene.total_time_, tuning);

	if (light.moon_intensity_ > 0.0 || night_factor > 0.0)
	{
		float3 moon_direction = normalize(light.sun_direction_);
		color += MoonDiscColor(view_direction, moon_direction, light.moon_angular_radius_, light.moon_color_ * max(light.moon_intensity_, 0.05), light.moon_phase_);
	}

	color += ShootingStarColor(view_direction, tuning);

	float alpha = saturate(dot(color, float3(0.333, 0.333, 0.333)));

	output[pixel] = float4(color, alpha);
}
