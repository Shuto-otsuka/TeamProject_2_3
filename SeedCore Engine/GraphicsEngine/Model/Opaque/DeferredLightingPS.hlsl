#include <GraphicsEngine/Shape/Fullscreen/Fullscreen.hlsli>
#include <GraphicsEngine/Light/Light.hlsli>
#include <GraphicsEngine/Light/LightShading.hlsli>
#include <GraphicsEngine/Model/Material/Material.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Environment/Weather/Weather.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

struct DeferredLightingResolveBindings
{
	uint geometry_depth_;
	uint material_surface_base_color_;
	uint material_surface_normal_roughness_;
	uint material_surface_tangent_;
	uint geometry_visibility_id_;
	uint geometry_visibility_texcoord_;
	uint geometry_instances_;
	uint light_constants_;
	uint weather_constants_;
	uint raytracing_shadow_visibility_;
	uint raytracing_ambient_occlusion_openness_;
	uint raytracing_subsurface_scattering_transmittance_;
	uint raytracing_reflection_radiance_;
	uint raytracing_global_illumination_radiance_;
	uint raytracing_refraction_radiance_;
	uint direct_light_punctual_radiance_;
	uint sky_diffuse_irradiance_;
	uint sky_specular_prefiltered_;
	uint sky_brdf_lut_;
};
ConstantBuffer<DeferredLightingResolveBindings> bindings : register(b0, space1);

/**
* [EN]
* DeferredLighting.Resolve: adds the light reaching the G-Buffer surface onto
* SurfaceRadiance, which Material already started with the emitted radiance.
* The sum is the sun through the surface's BRDF, scaled by the sun
* visibility in RaytracingShadow.Visibility.x (LoadSurfaceMaterial, with
* the texcoord's derivatives taken from the right and lower neighbours of the
* same instance, and Weather.Constants' wetness and snow cover), the sunlight seen through thin back-lit surfaces
* (RaytracingSubsurfaceScattering.Transmittance), the indirect light, the
* refracted light, the punctual lights' PunctualRadiance and a lightning
* strike's flash lighting the diffuse color. The indirect
* diffuse is the sky irradiance darkened by the occlusion (the ray-traced
* openness times the material's occlusion texture, and for fur the
* self-shadowing that fades from the skin to the tips), replaced by
* RaytracingGlobalIllumination.Radiance by its alpha. The indirect specular is
* the prefiltered sky darkened by the matching specular occlusion, replaced
* by RaytracingReflection.Radiance by its alpha (the reflection strength).
* The refracted light RaytracingRefraction.Radiance is weighted by its alpha,
* the transmission factor and the transmitted share of the Fresnel term.
* Unlit surfaces and the background receive nothing. The additive blend weights the output by its alpha, so
* the light is returned with alpha 1.
*
* ---------------------------------------------------------------------
*
* [JP]
* DeferredLighting.Resolve。G-Buffer の表面に届く光を、Material が放射を初期値
* として書いた SurfaceRadiance へ足す。足すのは、表面の BRDF
* （LoadSurfaceMaterial。UV の微分は同じインスタンスの右隣と下隣から求め、
* Weather.Constants の濡れと積雪を渡す）
* を通し RaytracingShadow.Visibility.x の太陽の可視率を掛けた太陽、薄く
* 裏から照らされた面を透ける太陽の光（RaytracingSubsurfaceScattering.
* Transmittance）、間接光、屈折光、点光源の PunctualRadiance、拡散色を照らす雷の
* 閃光。間接の拡散は
* 遮蔽（レイトレの開放度とマテリアルの occlusion テクスチャの積。Fur は根元から
* 毛先へ薄れる自己遮蔽も掛ける）で暗くした
* 空の放射照度で、RaytracingGlobalIllumination.Radiance をそのアルファの分
* だけ置き換える。間接の鏡面は対応するスペキュラオクルージョンで暗くした
* 事前フィルタの空で、RaytracingReflection.Radiance をそのアルファ（反射の
* 強さ）の分だけ置き換える。屈折光 RaytracingRefraction.Radiance は、その
* アルファ・透過係数・フレネルの透過分で重み付けする。Unlit の表面と背景には
* 何も足さない。加算ブレンドは出力をアルファで重み付けするので、光はアルファ 1
* で返す。
*/
float4 main(FullscreenOutput input) : SV_Target0
{
	uint2 pixel = uint2(input.position.xy);
	Texture2D<float> depth_texture = ResourceDescriptorHeap[bindings.geometry_depth_];
	float depth = depth_texture.Load(int3(pixel, 0));
	Texture2D<uint2> visibility_id = ResourceDescriptorHeap[bindings.geometry_visibility_id_];
	if (depth == 0.0 || visibility_id.Load(int3(pixel, 0)).x == 0)
	{
		return float4(0.0, 0.0, 0.0, 0.0);
	}

	uint instance_index;
	uint meshlet_index;
	uint triangle_in_meshlet_index;
	UnpackVisibilityID(visibility_id.Load(int3(pixel, 0)), instance_index, meshlet_index, triangle_in_meshlet_index);
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[instance_index];
	if (instance.shading_.shading_model_ == SHADING_MODEL_UNLIT)
	{
		return float4(0.0, 0.0, 0.0, 0.0);
	}

	uint width;
	uint height;
	depth_texture.GetDimensions(width, height);
	SceneConstantBuffer scene = GetSceneConstantBuffer();
	float2 uv = (float2(pixel) + 0.5) / float2(width, height);
	float4 world = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), scene.inverse_view_projection_);
	float3 view = normalize(scene.camera_position_.xyz - world.xyz / world.w);

	Texture2D<float4> surface_base_color = ResourceDescriptorHeap[bindings.material_surface_base_color_];
	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_surface_normal_roughness_];
	Texture2D<uint> surface_tangent = ResourceDescriptorHeap[bindings.material_surface_tangent_];
	Texture2D<float2> visibility_texcoord = ResourceDescriptorHeap[bindings.geometry_visibility_texcoord_];
	float4 base_color = surface_base_color.Load(int3(pixel, 0));
	uint normal_roughness = surface_normal_roughness.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(float2(normal_roughness & 0xFFF, (normal_roughness >> 12) & 0xFFF) / 4095.0);
	float roughness = (normal_roughness >> 24) / 255.0;

	uint2 pixel_x = uint2(min(pixel.x + 1, width - 1), pixel.y);
	uint2 pixel_y = uint2(pixel.x, min(pixel.y + 1, height - 1));
	float2 texcoord = visibility_texcoord.Load(int3(pixel, 0));
	float2 texcoord_dx = visibility_id.Load(int3(pixel_x, 0)).x == instance_index + 1 ? visibility_texcoord.Load(int3(pixel_x, 0)) - texcoord : float2(0.0, 0.0);
	float2 texcoord_dy = visibility_id.Load(int3(pixel_y, 0)).x == instance_index + 1 ? visibility_texcoord.Load(int3(pixel_y, 0)) - texcoord : float2(0.0, 0.0);

	ConstantBuffer<WeatherConstantBuffer> weather = ResourceDescriptorHeap[bindings.weather_constants_];
	SurfaceMaterial material = LoadSurfaceMaterial(base_color.rgb, base_color.a, roughness, normal, view, instance, texcoord, texcoord_dx, texcoord_dy, surface_tangent.Load(int3(pixel, 0)), weather.wetness_, weather.snow_coverage_);

	uint mode = scene.view_mode_;
	if (mode == VIEW_MODE_DETAIL_LIGHTING)
	{
		material.diffuse_color_ = float3(0.5, 0.5, 0.5);
		material.f0_ = float3(0.04, 0.04, 0.04);
	}
	else if (mode == VIEW_MODE_LIGHTING_ONLY)
	{
		material.diffuse_color_ = float3(1.0, 1.0, 1.0);
		material.f0_ = float3(0.04, 0.04, 0.04);
	}
	else if (mode == VIEW_MODE_REFLECTIONS)
	{
		material.diffuse_color_ = float3(0.0, 0.0, 0.0);
		material.f0_ = float3(1.0, 1.0, 1.0);
	}
	else if (mode == VIEW_MODE_DIRECT_DIFFUSE)
	{
		material.f0_ = float3(0.0, 0.0, 0.0);
	}
	else if (mode == VIEW_MODE_DIRECT_SPECULAR)
	{
		material.diffuse_color_ = float3(0.0, 0.0, 0.0);
	}

	ConstantBuffer<LightConstantBuffer> constants = ResourceDescriptorHeap[bindings.light_constants_];
	float3 lighting = float3(0.0, 0.0, 0.0);
	if (constants.sun_intensity_ > 0.0 && (scene.show_flags_low_ & SHOW_FLAG_DIRECT_LIGHT) != 0)
	{
		float3 sun_direction = normalize(-constants.sun_direction_);
		float3 sun_radiance = constants.sun_color_ * constants.sun_intensity_;
		Texture2D<float2> shadow_visibility = ResourceDescriptorHeap[bindings.raytracing_shadow_visibility_];
		lighting += shadow_visibility.Load(int3(pixel, 0)).x * EvalDirectLightDispatch(instance.shading_.shading_model_, normal, view, sun_direction, material.diffuse_color_, material.f0_, material.roughness_, material.clearcoat_factor_, material.clearcoat_roughness_, material.clearcoat_normal_, material.sheen_color_, material.sheen_roughness_, material.anisotropy_tangent_, material.anisotropy_bitangent_, material.anisotropy_strength_) * sun_radiance;

		Texture2D<float4> subsurface_transmittance = ResourceDescriptorHeap[bindings.raytracing_subsurface_scattering_transmittance_];
		lighting += material.diffuse_color_ * subsurface_transmittance.Load(int3(pixel, 0)).rgb * sun_radiance * saturate(dot(-normal, sun_direction));
	}

	Texture2D<float> ambient_occlusion_openness = ResourceDescriptorHeap[bindings.raytracing_ambient_occlusion_openness_];
	float occlusion = ambient_occlusion_openness.Load(int3(pixel, 0));
	if (instance.texture_.occlusion_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> occlusion_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.occlusion_texture_index_)];
		occlusion *= occlusion_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).r;
	}
	if (instance.shading_.shading_model_ == SHADING_MODEL_FUR)
	{
		occlusion *= lerp(0.35, 1.0, (float)instance.geometry_.layer_ / (float)max(instance.shading_.fur_shell_count_, 1));
	}
	float normal_dot_view = saturate(dot(normal, view));
	float specular_occlusion = saturate(pow(normal_dot_view + occlusion, exp2(-16.0 * material.roughness_ - 1.0)) - 1.0 + occlusion);
	float3 fresnel = material.f0_ + (max(1.0 - material.roughness_, material.f0_) - material.f0_) * pow(1.0 - normal_dot_view, 5.0);

	float3 image_based_diffuse = float3(0.0, 0.0, 0.0);
	float3 image_based_specular = float3(0.0, 0.0, 0.0);
	if (constants.sky_intensity_ > 0.0)
	{
		image_based_diffuse = ImageBasedLightingRadianceLambertian(bindings.sky_brdf_lut_, bindings.sky_diffuse_irradiance_, normal, view, material.roughness_, material.diffuse_color_, material.f0_) * occlusion * constants.sky_intensity_;
		image_based_specular = ImageBasedLightingRadianceGgx(bindings.sky_brdf_lut_, bindings.sky_specular_prefiltered_, normal, view, material.roughness_, material.f0_) * specular_occlusion * constants.sky_intensity_;
	}

	Texture2D<float4> reflection_radiance = ResourceDescriptorHeap[bindings.raytracing_reflection_radiance_];
	float4 reflection = reflection_radiance.Load(int3(pixel, 0));
	if (reflection.a > 0.0)
	{
		image_based_specular = lerp(image_based_specular, reflection.rgb * fresnel * max(reflection.a, 1.0), min(reflection.a, 1.0));
	}

	Texture2D<float4> global_illumination_radiance = ResourceDescriptorHeap[bindings.raytracing_global_illumination_radiance_];
	float4 global_illumination = global_illumination_radiance.Load(int3(pixel, 0));
	float global_illumination_weight = saturate(global_illumination.a);
	float3 indirect_diffuse = image_based_diffuse * (1.0 - global_illumination_weight) + global_illumination.rgb * global_illumination_weight * material.diffuse_color_;

	Texture2D<float4> refraction_radiance = ResourceDescriptorHeap[bindings.raytracing_refraction_radiance_];
	float4 refraction = refraction_radiance.Load(int3(pixel, 0));
	float3 indirect_specular = image_based_specular + refraction.rgb * (1.0 - fresnel) * refraction.a * saturate(instance.extension_.transmission_factor_);

	Texture2D<float4> punctual_radiance = ResourceDescriptorHeap[bindings.direct_light_punctual_radiance_];
	lighting += punctual_radiance.Load(int3(pixel, 0)).rgb;

	lighting += material.diffuse_color_ * weather.thunder_flash_ * float3(0.85, 0.9, 1.0) * 2.5;

	if (mode == VIEW_MODE_DIRECT_DIFFUSE || mode == VIEW_MODE_DIRECT_SPECULAR)
	{
		return float4(lighting, 1.0);
	}
	if (mode == VIEW_MODE_INDIRECT_DIFFUSE)
	{
		return float4(indirect_diffuse, 1.0);
	}
	if (mode == VIEW_MODE_INDIRECT_SPECULAR || mode == VIEW_MODE_REFLECTIONS)
	{
		return float4(indirect_specular, 1.0);
	}
	return float4(lighting + indirect_diffuse + indirect_specular, 1.0);
}
