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

/**
* [EN]
* Sky.Resolve: draws the background into Sky.Radiance for every pixel with
* no geometry. With a skymap the background is the cube itself. Without one
* (dispatch constant 1) it is the procedural sky: the zenith/horizon gradient
* with the sun disc, plus VolumetricStar.Radiance (stars lie behind the
* clouds, so the cloud coverage hides them), then VolumetricCloud.Radiance
* blended over it (rgb is already weighted by transmittance, a is the
* coverage). A lightning strike brightens the whole sky for a moment and
* draws the bolt near the peak of the flash: a jagged line of ten segments
* from the top of the screen, each shifted sideways by a hash of the strike's
* seed, glowing with the pixel's distance to the nearest segment. With
* neither (dispatch constant 2) the background is a flat 0.2 gray.
*
* ---------------------------------------------------------------------
*
* [JP]
* Sky.Resolve。ジオメトリの無いピクセルの背景を Sky.Radiance へ描く。スカイマップが
* あればキューブそのもの。無いとき（ディスパッチの定数 1）はプロシージャル空で、
* 天頂と地平のグラデーションに太陽の円盤、VolumetricStar.Radiance を足し（星は雲の
* 奥にあるので雲のカバレッジで隠れる）、その上に VolumetricCloud.Radiance を重ねる
* （rgb は透過率を掛け済み、a はカバレッジ）。雷は空全体を一瞬明るくし、閃光の
* ピークの近くで稲妻を描く。稲妻は画面の上から伸びる 10 本の線分の折れ線で、
* 各線分は雷の種のハッシュで横にずらし、いちばん近い線分までの距離で光らせる。
* どちらも無いとき（ディスパッチの定数 2）は、背景を 0.2 の灰色一色にする。
*/
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

	/// [EN] With neither a skymap nor the procedural sky (dispatch constant 2) the background is a flat 0.2 gray.
	/// [JP] スカイマップもプロシージャル空も無いとき（ディスパッチの定数 2）は、背景を 0.2 の灰色一色にする。
	if (dispatch_bindings.constants_ == 2)
	{
		radiance[id.xy] = float4(0.2, 0.2, 0.2, 1.0);
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	float2 uv = (float2(id.xy) + 0.5) / float2(width, height);
	float4 near_point = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0, 1.0), scene.inverse_view_projection_);
	float3 direction = normalize(near_point.xyz / near_point.w - scene.camera_position_.xyz);

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
