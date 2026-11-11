#include <GraphicsEngine/Shape/Fullscreen/Fullscreen.hlsli>
#include <GraphicsEngine/Light/Light.hlsli>
#include <GraphicsEngine/Light/LightShading.hlsli>
#include <GraphicsEngine/Model/Material/Material.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>

struct DeferredLightingDrawBindings
{
	uint geometry_depth_;
	uint material_surface_base_color_;
	uint material_surface_normal_roughness_;
	uint material_surface_tangent_;
	uint geometry_visibility_id_;
	uint geometry_visibility_texcoord_;
	uint model_instances_;
	uint light_constants_;
	uint direct_light_punctual_radiance_;
	uint sky_diffuse_irradiance_;
	uint sky_specular_prefiltered_;
	uint sky_brdf_lut_;
};
ConstantBuffer<DeferredLightingDrawBindings> bindings : register(b0, space1);

/**
* [EN]
* DeferredLighting.Draw: adds the light reaching the G-Buffer surface onto
* SurfaceRadiance, which Material already started with the emitted radiance.
* The sum is the sun through the surface's BRDF (LoadSurfaceMaterial, with
* the texcoord's derivatives taken from the right and lower neighbours of the
* same instance), image-based lighting scaled by the sky light's intensity
* (diffuse irradiance darkened by the material's occlusion texture, and the
* prefiltered specular darkened by the matching specular occlusion), and the
* punctual lights' PunctualRadiance. Unlit surfaces and the background
* receive nothing. The additive blend weights the output by its alpha, so
* the light is returned with alpha 1.
*
* ---------------------------------------------------------------------
*
* [JP]
* DeferredLighting.Draw。G-Buffer の表面に届く光を、Material が放射を初期値
* として書いた SurfaceRadiance へ足す。足すのは、表面の BRDF
* （LoadSurfaceMaterial。UV の微分は同じインスタンスの右隣と下隣から求める）
* を通した太陽、空の光の強さを掛けた IBL（拡散の放射照度はマテリアルの
* occlusion テクスチャで暗くし、事前フィルタの鏡面は対応するスペキュラ
* オクルージョンで暗くする）、点光源の PunctualRadiance。Unlit の表面と背景には
* 何も足さない。加算ブレンドは出力をアルファで重み付けするので、光はアルファ 1
* で返す。
*/
float4 main(FullscreenOutput input) : SV_Target0
{
	uint2 pixel = uint2(input.position.xy);
	Texture2D<float> depth_texture = ResourceDescriptorHeap[bindings.geometry_depth_];
	float depth = depth_texture.Load(int3(pixel, 0));
	if (depth == 0.0)
	{
		return float4(0.0, 0.0, 0.0, 0.0);
	}

	Texture2D<uint2> visibility_id = ResourceDescriptorHeap[bindings.geometry_visibility_id_];
	uint instance_index;
	uint meshlet_index;
	uint triangle_in_meshlet_index;
	UnpackVisibilityID(visibility_id.Load(int3(pixel, 0)), instance_index, meshlet_index, triangle_in_meshlet_index);
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.model_instances_];
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
	float2 texcoord_dx = visibility_id.Load(int3(pixel_x, 0)).x == instance_index ? visibility_texcoord.Load(int3(pixel_x, 0)) - texcoord : float2(0.0, 0.0);
	float2 texcoord_dy = visibility_id.Load(int3(pixel_y, 0)).x == instance_index ? visibility_texcoord.Load(int3(pixel_y, 0)) - texcoord : float2(0.0, 0.0);

	SurfaceMaterial material = LoadSurfaceMaterial(base_color.rgb, base_color.a, roughness, normal, view, instance, texcoord, texcoord_dx, texcoord_dy, surface_tangent.Load(int3(pixel, 0)), 0.0, 0.0);

	ConstantBuffer<LightConstantBuffer> constants = ResourceDescriptorHeap[bindings.light_constants_];
	float3 lighting = float3(0.0, 0.0, 0.0);
	if (constants.sun_intensity_ > 0.0)
	{
		float3 sun_direction = normalize(-constants.sun_direction_);
		lighting += EvalDirectLightDispatch(instance.shading_.shading_model_, normal, view, sun_direction, material.diffuse_color_, material.f0_, material.roughness_, material.clearcoat_factor_, material.clearcoat_roughness_, material.clearcoat_normal_, material.sheen_color_, material.sheen_roughness_, material.anisotropy_tangent_, material.anisotropy_bitangent_, material.anisotropy_strength_) * constants.sun_color_ * constants.sun_intensity_;
	}

	if (constants.sky_intensity_ > 0.0)
	{
		float occlusion = 1.0;
		if (instance.texture_.occlusion_texture_index_ != MODEL_INVALID)
		{
			Texture2D<float4> occlusion_texture = ResourceDescriptorHeap[instance.texture_.occlusion_texture_index_];
			occlusion = occlusion_texture.SampleGrad(sampler_linear_wrap, texcoord, texcoord_dx, texcoord_dy).r;
		}
		float normal_dot_view = saturate(dot(normal, view));
		float specular_occlusion = saturate(pow(normal_dot_view + occlusion, exp2(-16.0 * material.roughness_ - 1.0)) - 1.0 + occlusion);
		float3 image_based_diffuse = ImageBasedLightingRadianceLambertian(bindings.sky_brdf_lut_, bindings.sky_diffuse_irradiance_, normal, view, material.roughness_, material.diffuse_color_, material.f0_) * occlusion;
		float3 image_based_specular = ImageBasedLightingRadianceGgx(bindings.sky_brdf_lut_, bindings.sky_specular_prefiltered_, normal, view, material.roughness_, material.f0_) * specular_occlusion;
		lighting += (image_based_diffuse + image_based_specular) * constants.sky_intensity_;
	}

	Texture2D<float4> punctual_radiance = ResourceDescriptorHeap[bindings.direct_light_punctual_radiance_];
	lighting += punctual_radiance.Load(int3(pixel, 0)).rgb;

	return float4(lighting, 1.0);
}
