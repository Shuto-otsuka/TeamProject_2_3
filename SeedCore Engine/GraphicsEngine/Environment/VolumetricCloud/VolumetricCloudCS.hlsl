#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloud.hlsli>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudNoise.hlsli>
#include <GraphicsEngine/Light/Light.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>

struct VolumetricCloudShapeBindings
{
	uint volumetric_cloud_shape_;
};
ConstantBuffer<VolumetricCloudShapeBindings> shape_bindings : register(b0, space1);

struct VolumetricCloudDetailBindings
{
	uint volumetric_cloud_detail_;
};
ConstantBuffer<VolumetricCloudDetailBindings> detail_bindings : register(b0, space1);

struct VolumetricCloudTraceBindings
{
	uint volumetric_cloud_constants_;
	uint volumetric_cloud_shape_;
	uint volumetric_cloud_detail_;
	uint geometry_depth_;
	uint light_constants_;
	uint volumetric_cloud_radiance_;
};
ConstantBuffer<VolumetricCloudTraceBindings> trace_bindings : register(b0, space1);

/**
* [EN]
* VolumetricCloud.Build.Shape: bakes the cloud shape noise (low frequency)
* into a Texture3D once. Instead
* of the runtime raymarch computing Fbm/Worley every step, it just samples
* this pre-baked texture through the wrap sampler (tileable, so no seams).
* The content is Perlin-Worley - inverted Worley's "puffy blobs" modulated by
* Perlin's large-scale billowing. Inverted Worley alone reads as a foam of
* equal-sized bubbles with no sense of connected cloud masses.
*
* 128^3 at base_cells=4 with 4 octaves (4/8/16/32 cells). Even the top octave
* keeps 4 voxels per cell - packing it tighter would bake to aliased hash
* noise instead of usable detail.
*
* ---------------------------------------------------------------------
*
* [JP]
* VolumetricCloud.Build.Shape。雲の形状ノイズ(低周波)を Texture3D へ一度だけベイクする。実行時のレイマーチ
* が毎ステップ Fbm/Worley を計算する代わりに、この焼き込み済みテクスチャを
* wrap サンプラーで読むだけで済むようにする(タイル可能なので継ぎ目なし)。
* 中身は Perlin-Worley - 反転 Worley の「もこもこの塊」に Perlin の大きな
* うねりを掛けたもの。反転 Worley 単体だと同じ大きさの粒が一面に並んだ
* 泡状の見た目になり、雲の塊としてのつながりが出ないため。
*
* 128^3 に対して base_cells=4 の 4 オクターブ(4/8/16/32セル)。最上位
* オクターブでも 1 セル 4 ボクセルを確保している - ここを詰めるとベイク時点で
* エイリアスしたハッシュノイズになる。
*/
[numthreads(4, 4, 4)]
void Shape(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture3D<float> shape_output = ResourceDescriptorHeap[shape_bindings.volumetric_cloud_shape_];

	uint width, height, depth;
	shape_output.GetDimensions(width, height, depth);

	if (dtid.x >= width || dtid.y >= height || dtid.z >= depth)
	{
		return;
	}

	float3 uvw = (float3(dtid) + 0.5) / float3(width, height, depth);

	shape_output[dtid] = TileablePerlinWorley(uvw, 4.0, 4);
}

/**
* [EN]
* VolumetricCloud.Build.Detail: bakes the cloud detail noise (high
* frequency) into a Texture3D once - a
* finer inverted Worley FBM than the shape noise, used to erode the cloud
* silhouette and carve puffy edges. Edge erosion is the Worley family's job,
* so no Perlin is mixed in here; this stays pure inverted Worley.
*
* 64^3 at base_cells=4 with 3 octaves (4/8/16 cells). 4 voxels per cell even
* at the top octave - keeping several voxels per cell even at the finest
* octave avoids aliasing.
*
* ---------------------------------------------------------------------
*
* [JP]
* VolumetricCloud.Build.Detail。雲のディテールノイズ(高周波)を Texture3D へ一度だけベイクする。形状ノイズ
* より細かい反転 Worley FBM で、雲の輪郭を侵食してもこもこの縁を作るのに使う。
* 縁の侵食は Worley 系の担当なので、こちらは Perlin を混ぜず反転 Worley のまま。
*
* 64^3 に対して base_cells=4 の 3 オクターブ(4/8/16セル)。最上位オクターブで
* 1 セル 4 ボクセル - エイリアスを避けるため最上位でもセルあたり複数ボクセルを
* 確保する。
*/
[numthreads(4, 4, 4)]
void Detail(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture3D<float> detail_output = ResourceDescriptorHeap[detail_bindings.volumetric_cloud_detail_];

	uint width, height, depth;
	detail_output.GetDimensions(width, height, depth);

	if (dtid.x >= width || dtid.y >= height || dtid.z >= depth)
	{
		return;
	}

	float3 uvw = (float3(dtid) + 0.5) / float3(width, height, depth);

	detail_output[dtid] = TileableWorleyFbm(uvw, 4.0, 3);
}

/**
* [EN]
* Reference:
* - https://advances.realtimerendering.com/s2015/The%20Real-time%20Volumetric%20Cloudscapes%20of%20Horizon%20-%20Zero%20Dawn%20-%20ARTR.pdf
*   (Schneider & Vos, "The Real-time Volumetric Cloudscapes of Horizon: Zero
*   Dawn", SIGGRAPH 2015 Advances in Real-Time Rendering - the curved shell +
*   weather-map + adaptive-stepping raymarch this pass is built on.)
* - https://advances.realtimerendering.com/s2017/Nubis%20-%20Authoring%20Realtime%20Volumetric%20Cloudscapes%20with%20the%20Decima%20Engine%20-%20Final%20.pdf
*   (Schneider, "Nubis: Authoring Real-Time Volumetric Cloudscapes with the
*   Decima Engine", SIGGRAPH 2017 - the follow-up covering the detail-erosion
*   and weather-authoring pipeline CloudDensity implements.)
* - https://history.siggraph.org/wp-content/uploads/2022/10/2015-Talks-Wrenninge_Art-Directable-Multiple-Volumetric-Scattering.pdf
*   (Wrenninge et al., "Art-Directable Multiple Volumetric Scattering",
*   SIGGRAPH 2015 - the octave-stack multiple-scattering approximation
*   CloudMultiScatterSetup/CloudSunLight implement, in VolumetricCloud.hlsli.)
*
* VolumetricCloud.Trace: volumetric cloud scapes (screen-space raymarch). Marches the view
* ray through a curved cloud SHELL (a layer wrapped around a planet of
* planet_radius_, so it bends down and converges at the horizon instead of
* extending forever as a flat slab), sampling PRE-BAKED tileable Texture3Ds
* (Perlin-Worley shape 128^3 + Worley detail 64^3, baked once by
* VolumetricCloud.Build). A low-frequency slice of the shape volume
* acts as a weather map so the sky gets large cloud masses and clear gaps.
* Stepping is adaptive: coarse strides skip empty space, fine strides integrate
* inside cloud, and both grow with distance in place of a mip chain. Lighting is
* a dual-lobe phase with a Wrenninge multiple-scattering octave stack, a
* Beer-powder term, a geometrically-stepped sun lightmarch that follows the real
* path length through the layer, and a sky-gradient ambient term. Distant clouds
* wash toward the horizon color (aerial perspective) and fade out at the march
* limit. Writes VolumetricCloud.Radiance (rgb the scattered light already
* weighted by transmittance, a the coverage); SkyResolveCS.hlsl blends it over
* the procedural sky, which is drawn only when no skymap is bound. Sky pixels
* only, no TLAS.
*
* ---------------------------------------------------------------------
*
* [JP]
* VolumetricCloud.Trace。ボリューメトリック・クラウドスケープ(スクリーン空間レイマーチ)。
* 視線を【曲率つきの雲シェル】(planet_radius_ の球殻。無限平板ではないので
* 遠方で下がって地平線に収束する)に沿ってマーチし、【ベイク済み】タイル可能
* Texture3D(Perlin-Worley 形状 128^3 + Worley ディテール 64^3、
* VolumetricCloud.Build が一度だけ焼く)をサンプルする。形状
* ボリュームの低周波スライスを weather map として使い、空に大きな雲の塊と
* 晴れ間を作る。ステップは適応的 - 空(密度0)は粗ステップで飛ばし、雲の中だけ
* 細ステップで積分し、どちらも距離に応じて伸ばして mip の代わりにする。
* ライティングはデュアルローブ位相 + Wrenninge の多重散乱オクターブ +
* Beer-powder + 層内の実光路長に沿った指数ステップのライトマーチ +
* 空グラデーション環境光。遠景の雲は地平線色へ寄せ(空気遠近)、マーチ上限へ
* 向けてフェードアウトする。VolumetricCloud.Radiance(rgb は透過率を掛け済みの
* 散乱光、a はカバレッジ)へ書き、SkyResolveCS.hlsl がプロシージャル空の上に
* 合成する(プロシージャル空はスカイマップが無いときだけ描く)。空ピクセル限定、
* TLAS 不使用。
*/

[numthreads(8, 8, 1)]
void Trace(uint3 dtid : SV_DispatchThreadID)
{
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	RWTexture2D<float4> output = ResourceDescriptorHeap[trace_bindings.volumetric_cloud_radiance_];
	uint cloud_width;
	uint cloud_height;
	output.GetDimensions(cloud_width, cloud_height);
	if (dtid.x >= cloud_width || dtid.y >= cloud_height)
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

	ConstantBuffer<VolumetricCloudConstantBuffer> tuning = ResourceDescriptorHeap[trace_bindings.volumetric_cloud_constants_];

	Texture3D<float> shape_noise = ResourceDescriptorHeap[trace_bindings.volumetric_cloud_shape_];
	Texture3D<float> detail_noise = ResourceDescriptorHeap[trace_bindings.volumetric_cloud_detail_];

	float2 uv = (float2(pixel) + 0.5) / float2(cloud_width, cloud_height);
	float2 ndc = float2(uv.x * 2 - 1, 1 - uv.y * 2);
	float4 far_clip = float4(ndc, 0.0, 1.0);
	float4 far_world = mul(far_clip, scene.inverse_view_projection_);
	float3 ray_direction = normalize(far_world.xyz / far_world.w - scene.camera_position_.xyz);
	float3 ray_origin = scene.camera_position_.xyz;

	float t_enter;
	float t_exit;
	if (!CloudLayerInterval(ray_origin, ray_direction, tuning, t_enter, t_exit))
	{
		output[pixel] = float4(0, 0, 0, 0);
		return;
	}

	ConstantBuffer<LightConstantBuffer> light = ResourceDescriptorHeap[trace_bindings.light_constants_];
	float3 light_direction = normalize(-light.sun_direction_);
	float3 sun_radiance = light.sun_color_ * light.sun_intensity_;

	float3 cloud_albedo = lerp(tuning.cloud_albedo_, tuning.cloud_albedo_ * 0.35, saturate(tuning.rain_));

	float curvature = CloudCurvature(ray_direction, tuning.planet_radius_);
	float layer_thickness = max(tuning.cloud_top_ - tuning.cloud_bottom_, 1.0);
	float cos_theta = dot(ray_direction, light_direction);

	CloudMultiScatter multi_scatter = CloudMultiScatterSetup(cos_theta, tuning);

	float jitter = frac(InterleavedGradientNoise(float2(pixel)) + tuning.temporal_jitter_ * float(tuning.frame_index_ & 63u) * 0.6180339887);

	const uint max_iterations = 256;

	float fine_step_base = layer_thickness / float(max(tuning.step_count_, 1u));
	const float coarse_step_scale = 8.0;

	float minimum_step = (t_exit - t_enter) / (float(max_iterations) * 1.5);

	float3 scattering = float3(0, 0, 0);
	float transmittance = 1.0;

	float weighted_distance = 0.0;
	float weight_total = 0.0;

	float t = t_enter;
	bool coarse = true;
	uint empty_run = 0;

	for (uint iteration = 0; iteration < max_iterations; iteration++)
	{
		if (t >= t_exit)
		{
			break;
		}

		float lod = 1.0 + t / max(tuning.lod_distance_, 1.0);
		float fine_step = max(fine_step_base * lod, minimum_step);
		float step_length = coarse ? fine_step * coarse_step_scale : fine_step;

		float sample_t = t + step_length * (coarse ? 0.5 : jitter);
		if (sample_t >= t_exit)
		{
			break;
		}

		float altitude = CloudAltitudeAt(ray_origin.y, ray_direction.y, curvature, sample_t);
		float3 sample_position = ray_origin + ray_direction * sample_t;

		float detail_strength = coarse ? 0.0 : saturate(1.0 - (t - tuning.lod_distance_) / max(tuning.lod_distance_ * 3.0, 1.0));

		float density = CloudDensity(sample_position, altitude, tuning, scene.total_time_, shape_noise, detail_noise, sampler_linear_wrap, detail_strength);

		if (density <= 0.0)
		{
			empty_run++;
			if (!coarse && empty_run > 8)
			{
				coarse = true;
				empty_run = 0;
			}
			t += step_length;
			continue;
		}

		empty_run = 0;

		if (coarse)
		{
			coarse = false;
			continue;
		}

		uint light_steps = transmittance > 0.3 ? tuning.light_step_count_ : max(tuning.light_step_count_ / 2u, 2u);

		float light_optical_depth = CloudLightOpticalDepth(sample_position, altitude, light_direction, tuning, scene.total_time_, shape_noise, detail_noise, sampler_linear_wrap, light_steps, jitter);

		float sample_extinction = density * step_length;

		float powder = CloudPowder(light_optical_depth, cos_theta, tuning.powder_strength_);
		float sun_light = CloudSunLight(light_optical_depth, powder, multi_scatter);

		float height01 = saturate((altitude - tuning.cloud_bottom_) / layer_thickness);
		float ambient_occlusion = lerp(0.3, 1.0, height01);
		float3 sky_ambient = lerp(tuning.sky_horizon_color_, tuning.sky_zenith_color_, height01) * tuning.sky_brightness_ * tuning.ambient_strength_ * ambient_occlusion;

		float3 in_scattering = cloud_albedo * (sun_radiance * sun_light + sky_ambient);

		float sample_transmittance = exp(-sample_extinction);
		float segment_weight = (1.0 - sample_transmittance) * transmittance;

		scattering += in_scattering * segment_weight;

		weighted_distance += sample_t * segment_weight;
		weight_total += segment_weight;

		transmittance *= sample_transmittance;

		if (transmittance < 0.005)
		{
			break;
		}

		t += step_length;
	}

	float alpha = 1.0 - transmittance;

	if (alpha > 0.0)
	{
		float mean_distance = weight_total > 0.0 ? weighted_distance / weight_total : t_enter;

		float aerial = exp(-mean_distance * tuning.aerial_density_);
		float3 horizon = tuning.sky_horizon_color_ * tuning.sky_brightness_;
		scattering = lerp(horizon * alpha, scattering, aerial);

		float fade_start = tuning.max_march_distance_ * 0.6;
		float distance_fade = 1.0 - smoothstep(fade_start, tuning.max_march_distance_, mean_distance);

		alpha *= distance_fade;
		scattering *= distance_fade;
	}

	output[pixel] = float4(scattering, alpha);
}
