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

/**
* [EN]
* Reference:
* - https://iquilezles.org/articles/distfunctions2d/
*   (Inigo Quilez, "2D Distance Functions" - the exact N-pointed star SDF
*   (VolumetricStar.hlsli's StarSDF) this pass's star field is built from.)
*
* VolumetricStar.Trace (screen-space): draws the procedural star field, the
* moon disc and any active shooting star streaks into VolumetricStar.Radiance,
* which SkyResolveCS.hlsl adds to the procedural sky before blending the
* clouds over it. Sky pixels only, no TLAS. Reads the night factor and the
* moon from Light.Constants; the moon sits exactly opposite the sun, so its
* direction is sun_direction_ itself (the direction the sunlight travels).
*
* [JP]
* VolumetricStar.Trace(スクリーン空間): プロシージャル星空・月ディスク・
* アクティブな流れ星の筋を VolumetricStar.Radiance へ描き、SkyResolveCS.hlsl が
* プロシージャル空に足してから雲を重ねる。空ピクセル限定、TLAS 不使用。夜の
* 強さと月は Light.Constants から読む。月は太陽の真反対にあるので、向きは
* sun_direction_(太陽の光が進む向き)そのもの。
*/

[numthreads(8, 8, 1)]
void Trace(uint3 dtid : SV_DispatchThreadID)
{
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	RWTexture2D<float4> output = ResourceDescriptorHeap[trace_bindings.volumetric_star_radiance_];

	uint star_width;
	uint star_height;
	output.GetDimensions(star_width, star_height);

	/// [EN] Bounds guard: the dispatch is rounded up to a multiple of the
	///      8x8 thread group size, so threads past the actual output
	///      texture's edge must bail out before touching any resource.
	/// [JP] 範囲外ガード: ディスパッチは 8x8 スレッドグループの倍数に
	///      切り上げられているので、実際の出力テクスチャ端を超えた
	///      スレッドはどのリソースにも触れる前に抜ける必要がある。
	if (dtid.x >= star_width || dtid.y >= star_height)
	{
		return;
	}

	uint2 pixel = dtid.xy;

	/// [EN] Skip any pixel with scene geometry (sky pixels only).
	/// [JP] シーンジオメトリがあるピクセルはスキップ(空ピクセル限定)。
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

	/// [EN] Reconstruct the view direction for this pixel by unprojecting a
	///      point on the far plane (device depth 0.0 under reverse-Z) and
	///      subtracting the camera position - no G-Buffer depth is needed
	///      since this pass only ever touches sky pixels.
	/// [JP] far 平面上の点(reverse-Z では device depth 0.0)を逆投影し、
	///      カメラ位置を引いてこのピクセルの視線方向を復元する - この
	///      パスは空ピクセルしか触らないので G-Buffer 深度は不要。
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

	/// [EN] Alpha follows the drawn color's own luminance rather than a
	///      binary mask, so a faint twinkle or the soft glow around a star
	///      composites with a matching soft edge instead of a hard cutout.
	/// [JP] アルファは二値マスクではなく描いた色自身の輝度に追従させる。
	///      弱い瞬きや星まわりの淡いグロウが、硬い切り抜きではなく
	///      なめらかな縁で合成されるようにするため。
	float alpha = saturate(dot(color, float3(0.333, 0.333, 0.333)));

	output[pixel] = float4(color, alpha);
}
