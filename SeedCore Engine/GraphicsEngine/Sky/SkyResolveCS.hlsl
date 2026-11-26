#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>
#include <GraphicsEngine/Light/Light.hlsli>
#include <GraphicsEngine/Environment/Weather/Weather.hlsli>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloud.hlsli>

struct SkyResolveBindings
{
	uint sky_cubemap_;
	uint geometry_depth_;
	uint volumetric_cloud_constants_;
	uint volumetric_cloud_radiance_;
	uint volumetric_star_radiance_;
	uint weather_constants_;
	uint light_constants_;
	uint sky_radiance_;
};
ConstantBuffer<SkyResolveBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<float4> radiance = ResourceDescriptorHeap[bindings.sky_radiance_];
	uint width;
	uint height;
	radiance.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	Texture2D<float> depth = ResourceDescriptorHeap[bindings.geometry_depth_];
	if (depth.Load(int3(id.xy, 0)) != 0.0)
	{
		radiance[id.xy] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	if (dispatch_bindings.constants_ == 2)
	{
		radiance[id.xy] = float4(0.2, 0.2, 0.2, 1.0);
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	float2 uv = (float2(id.xy) + 0.5) / float2(width, height);
	float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
	float3 view_direction = float3((ndc.x - scene.projection_._31) / scene.projection_._11, (ndc.y - scene.projection_._32) / scene.projection_._22, 1.0);
	float3 direction = normalize(mul(view_direction, (float3x3)scene.inverse_view_));

	float3 color;
	if (dispatch_bindings.constants_ != 0)
	{
		ConstantBuffer<VolumetricCloudConstantBuffer> tuning = ResourceDescriptorHeap[bindings.volumetric_cloud_constants_];
		ConstantBuffer<LightConstantBuffer> light = ResourceDescriptorHeap[bindings.light_constants_];
		color = ProceduralSkyColor(direction, normalize(-light.sun_direction_), light.sun_color_ * light.sun_intensity_, tuning);

		Texture2D<float4> stars = ResourceDescriptorHeap[bindings.volumetric_star_radiance_];
		color += stars.Load(int3(id.xy, 0)).rgb;

		Texture2D<float4> clouds = ResourceDescriptorHeap[bindings.volumetric_cloud_radiance_];
		float4 cloud = clouds.Load(int3(id.xy, 0));
		color = color * (1.0 - cloud.a) + cloud.rgb;
	}
	else
	{
		TextureCube<float4> cubemap = ResourceDescriptorHeap[bindings.sky_cubemap_];
		color = cubemap.SampleLevel(sampler_linear_clamp, direction, 0).rgb;
	}

	ConstantBuffer<WeatherConstantBuffer> weather = ResourceDescriptorHeap[bindings.weather_constants_];
	color += weather.thunder_flash_ * float3(0.85, 0.9, 1.0) * 2.5;
	float bolt_intensity = saturate(weather.thunder_flash_ - 0.5) * 2.0;
	if (bolt_intensity > 0.0001)
	{
		float2 seed = float2(weather.thunder_seed_, weather.thunder_seed_ * 1.618034);
		float bolt_distance = 1e5;
		float x = IQHash(seed) * 0.8 + 0.1;
		float y = 0.0;
		for (int segment_index = 0; segment_index < 10; segment_index++)
		{
			float next_x = x + (IQHash(seed + float2(float(segment_index), 0.0)) * 2.0 - 1.0) * 0.05;
			float next_y = y + 0.1;
			float2 from_start = uv - float2(x, y);
			float2 segment = float2(next_x, next_y) - float2(x, y);
			float along = saturate(dot(from_start, segment) / dot(segment, segment));
			bolt_distance = min(bolt_distance, length(from_start - segment * along));
			x = next_x;
			y = next_y;
		}
		float glow = exp(-bolt_distance * 220.0) * bolt_intensity;
		float core = exp(-bolt_distance * 900.0) * bolt_intensity;
		color += float3(0.85, 0.9, 1.0) * (glow * 1.5 + core * 4.0);
	}

	radiance[id.xy] = float4(color, 1.0);
}
