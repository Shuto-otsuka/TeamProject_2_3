#include <GraphicsEngine/Sky/Sky.hlsli>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloud.hlsli>
#include <GraphicsEngine/Light/Light.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>

struct SkyProceduralBindings
{
	uint volumetric_cloud_constants_;
	uint volumetric_cloud_shape_;
	uint volumetric_cloud_detail_;
	uint light_constants_;
	uint sky_cubemap_;
};
ConstantBuffer<SkyProceduralBindings> bindings : register(b0, space1);

/**
* [EN]
* Sky.Build.Procedural: renders the procedural sky (gradient + sun, see
* VolumetricCloud.hlsli::ProceduralSkyColor) and the clouds onto the six
* faces of Sky.Cubemap so the irradiance/prefilter convolution that follows
* turns it into IBL - this is how the procedural sky lights the scene when no
* skymap is bound. The background itself is not read from this cube: Sky.Resolve
* draws the analytic sky (crisp sun disc) and the full-resolution clouds. The
* dispatch constant carries the scene time (as float bits) that scrolls the
* cloud wind.
*
* [JP]
* Sky.Build.Procedural。プロシージャル空(グラデーション+太陽、
* VolumetricCloud.hlsli::ProceduralSkyColor 参照)と雲を Sky.Cubemap の6面へ描き、
* 続く irradiance/prefilter 畳み込みで IBL 化する — スカイマップ未バインド時に
* プロシージャル空がシーンを照らす仕組み。背景そのものはこのキューブから読まず、
* Sky.Resolve が解析的な空(シャープな太陽)とフル解像度の雲を描く。ディスパッチの
* 定数は雲の風を流すシーンの時刻(float のビット列)。
*/
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	RWTexture2DArray<float4> destination = ResourceDescriptorHeap[bindings.sky_cubemap_];
	uint size;
	uint height;
	uint element_count;
	destination.GetDimensions(size, height, element_count);
	if (id.x >= size || id.y >= size)
	{
		return;
	}

	uint face = id.z;
	float2 uv = (float2(id.xy) + 0.5) / float(size) * 2.0 - 1.0;
	float3 direction = normalize(CubeFaceDirection(face, uv));

	ConstantBuffer<VolumetricCloudConstantBuffer> tuning = ResourceDescriptorHeap[bindings.volumetric_cloud_constants_];

	ConstantBuffer<LightConstantBuffer> light = ResourceDescriptorHeap[bindings.light_constants_];
	float3 sun_direction = normalize(-light.sun_direction_);
	float3 sun_radiance = light.sun_color_ * light.sun_intensity_;

	float3 color = ProceduralSkyColor(direction, sun_direction, sun_radiance, tuning);

	// [JP] 雲もキューブへ焼き込む(縮小版レイマーチ: 固定24ステップ+
	//      ライト3ステップ)。これで反射(prefilter経由)にも IBL にも雲が乗り、
	//      雨雲にすると地上の環境光も暗くなる。キャプチャ視点は地上原点
	//      (0,0,0)。時刻(風スクロール)はディスパッチの定数で渡される。
	//      IBL は低周波なので適応ステップまでは持ち込まないが、積分式・シェル
	//      形状・多重散乱はスクリーン側と揃える — でないとキューブ由来の環境光
	//      だけが別の明るさになる。
	if (direction.y > 0.001)
	{
		const uint capture_step_count = 24;
		const uint capture_light_step_count = 3;
		float capture_time = asfloat(dispatch_bindings.constants_);

		Texture3D<float> shape_noise = ResourceDescriptorHeap[bindings.volumetric_cloud_shape_];
		Texture3D<float> detail_noise = ResourceDescriptorHeap[bindings.volumetric_cloud_detail_];

		float3 ray_origin = float3(0.0, 0.0, 0.0);
		float layer_thickness = max(tuning.cloud_top_ - tuning.cloud_bottom_, 1.0);
		float curvature = CloudCurvature(direction, tuning.planet_radius_);

		float t_enter;
		float t_exit;
		if (CloudLayerInterval(ray_origin, direction, tuning, t_enter, t_exit))
		{
			// [JP] 固定ステップなので、区間が地平線側で数万単位まで伸びると
			//      1ステップがノイズの特徴を飛び越える。焼き込みでは区間を
			//      切ってステップ長を常識的な範囲に収める。
			t_exit = min(t_exit, t_enter + layer_thickness * 8.0);

			float3 cloud_albedo = lerp(tuning.cloud_albedo_, tuning.cloud_albedo_ * 0.35, saturate(tuning.rain_));

			float step_length = (t_exit - t_enter) / float(capture_step_count);
			float cos_theta = dot(direction, sun_direction);

			// [JP] 位相はこの方向の間ずっと定数なのでループ外で一度だけ作る。
			CloudMultiScatter multi_scatter = CloudMultiScatterSetup(cos_theta, tuning);

			float3 scattering = float3(0, 0, 0);
			float transmittance = 1.0;
			float t = t_enter + step_length * 0.5;

			for (uint step = 0; step < capture_step_count; step++)
			{
				float altitude = CloudAltitudeAt(ray_origin.y, direction.y, curvature, t);
				float3 sample_position = ray_origin + direction * t;
				float density = CloudDensity(sample_position, altitude, tuning, capture_time, shape_noise, detail_noise, sampler_linear_wrap, 1.0);

				if (density > 0.0)
				{
					float light_optical_depth = CloudLightOpticalDepth(sample_position, altitude, sun_direction,
						tuning, capture_time, shape_noise, detail_noise, sampler_linear_wrap,
						capture_light_step_count, 0.5);

					float sample_extinction = density * step_length;

					float powder = CloudPowder(light_optical_depth, cos_theta, tuning.powder_strength_);
					float sun_light = CloudSunLight(light_optical_depth, powder, multi_scatter);

					float height01 = saturate((altitude - tuning.cloud_bottom_) / layer_thickness);
					float ambient_occlusion = lerp(0.3, 1.0, height01);
					float3 sky_ambient = lerp(tuning.sky_horizon_color_, tuning.sky_zenith_color_, height01)
						* tuning.sky_brightness_ * tuning.ambient_strength_ * ambient_occlusion;

					float3 in_scattering = cloud_albedo * (sun_radiance * sun_light + sky_ambient);

					float sample_transmittance = exp(-sample_extinction);
					scattering += in_scattering * (1.0 - sample_transmittance) * transmittance;
					transmittance *= sample_transmittance;

					if (transmittance < 0.01)
					{
						break;
					}
				}

				t += step_length;
			}

			color = color * lerp(1.0, transmittance, saturate(tuning.ibl_intensity_)) + scattering * tuning.ibl_intensity_;
		}
	}

	destination[uint3(id.xy, face)] = float4(color, 1.0);
}
