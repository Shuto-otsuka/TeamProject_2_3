#include <GraphicsEngine/Model/Transparent/Transparent.hlsli>
#include <GraphicsEngine/Model/Material/Material.hlsli>
#include <GraphicsEngine/Light/LightShading.hlsli>
#include <GraphicsEngine/Raytracing/RaytracingHit.hlsli>
#include <GraphicsEngine/PostProcess/Outline/Outline.hlsli>

/**
* [EN]
* Shades one transparent fragment forward and adds it to its pixel's list.
* The material is read the same way the opaque material resolve reads it
* (mapping, base color, normal map, metallic and roughness, emission). The
* light is the sun, shadowed by a ray traced through TopLevel.Structure, the
* clustered punctual lights and the sky's image based lighting. The color is
* stored premultiplied by its alpha, together with the depth and this
* layer's motion, and linked in front of the pixel's head with one atomic
* exchange; fragments past the pool's capacity are dropped. When the
* instance's actor is outlined, the fragment also records itself in
* Outline.Mask as (depth << 17) | actor, so the nearest one per pixel
* stays.
*
* ---------------------------------------------------------------------
*
* [JP]
* 半透明のフラグメントを 1 つ前方で陰影付けし、そのピクセルのリストへ足す。
* マテリアルは不透明のマテリアル解決と同じ読み方をする（マッピング、
* ベースカラー、法線マップ、メタリックとラフネス、放射）。光は、
* TopLevel.Structure へ飛ばすレイで影を取った太陽、クラスタの点光源類、
* 空の IBL。色はアルファを掛けた形で、深度とこの層の動きと一緒に置き、
* ピクセルの先頭へ不可分な交換 1 回でつなぐ。プールの容量を超えた
* フラグメントは捨てる。インスタンスのアクターが縁取られるときは、
* (深度 << 17) | アクター として Outline.Mask にも記録し、ピクセルごとに
* 一番手前のものが残る。
*/
[earlydepthstencil]
void main(TransparentOutput input, bool front_face : SV_IsFrontFace)
{
	SceneConstantBuffer scene = GetSceneConstantBuffer();
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[input.instance_index];

	float3 normal = normalize(input.normal);
	float3 object_normal = normalize(input.object_normal);
	if (!front_face)
	{
		normal = -normal;
		object_normal = -object_normal;
	}
	float3 view = normalize(scene.camera_position_.xyz - input.world_position);
	float3 tangent = normalize(input.tangent.xyz);
	float tangent_sign = input.tangent.w;
	float3 object_tangent = normalize(input.object_tangent);
	float3 object_bitangent = cross(object_normal, object_tangent) * tangent_sign;
	float3 object_scale = float3(length(instance.transform_.world_[0].xyz), length(instance.transform_.world_[1].xyz), length(instance.transform_.world_[2].xyz));
	MaterialMappingCoordinate mapping = ResolveMaterialMapping(instance.shading_.material_mapping_, instance.shading_.tiling_, instance.shading_.triplanar_sharpness_, input.texcoord, input.object_position, object_normal, object_tangent, object_bitangent, object_scale, input.world_position, normal);

	float4 base_color = instance.texture_.base_color_;
	if (instance.texture_.base_color_texture_index_ != MODEL_INVALID)
	{
		Texture2D base_color_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.base_color_texture_index_)];
		base_color *= SampleMaterialMappingGrad(base_color_texture, sampler_linear_wrap, mapping);
	}
	if (base_color.a <= 0.0)
	{
		discard;
	}

	float3 shading_normal = normal;
	if (instance.texture_.normal_texture_index_ != MODEL_INVALID)
	{
		Texture2D normal_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.normal_texture_index_)];
		if (instance.shading_.material_mapping_ == MATERIAL_MAPPING_TRIPLANAR_OBJECT)
		{
			float3 projection_normal = SampleNormalMappingGrad(normal_texture, sampler_linear_wrap, mapping);
			shading_normal = normalize(mul(float4(projection_normal * object_scale, 0.0), instance.transform_.inverse_transpose_world_).xyz);
		}
		else if (instance.shading_.material_mapping_ == MATERIAL_MAPPING_TRIPLANAR_WORLD)
		{
			shading_normal = SampleNormalMappingGrad(normal_texture, sampler_linear_wrap, mapping);
		}
		else
		{
			float3 bitangent = cross(normal, tangent) * tangent_sign;
			float3 tangent_normal = SampleMaterialMappingGrad(normal_texture, sampler_linear_wrap, mapping).xyz * 2.0 - 1.0;
			shading_normal = normalize(mul(tangent_normal, float3x3(tangent, bitangent, normal)));
		}
	}

	float metallic = instance.texture_.metallic_;
	float roughness = instance.texture_.roughness_;
	if (instance.texture_.metallic_roughness_texture_index_ != MODEL_INVALID)
	{
		Texture2D metallic_roughness_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.metallic_roughness_texture_index_)];
		float4 metallic_roughness = SampleMaterialMappingGrad(metallic_roughness_texture, sampler_linear_wrap, mapping);
		metallic *= metallic_roughness.b;
		roughness *= metallic_roughness.g;
	}

	float3 emissive = instance.texture_.emissive_;
	if (instance.texture_.emissive_texture_index_ != MODEL_INVALID)
	{
		Texture2D emissive_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.emissive_texture_index_)];
		emissive *= SampleMaterialMappingGrad(emissive_texture, sampler_linear_wrap, mapping).rgb;
	}
	emissive *= instance.texture_.emissive_strength_;

	float3 radiance = base_color.rgb + emissive;
	if (instance.shading_.shading_model_ != SHADING_MODEL_UNLIT)
	{
		uint tangent_bits = uint(PackTangentAngle(shading_normal, tangent, tangent_sign) * 65535.0 + 0.5);
		ConstantBuffer<WeatherConstantBuffer> weather = ResourceDescriptorHeap[bindings.weather_constants_];
		SurfaceMaterial material = LoadSurfaceMaterial(base_color.rgb, metallic, roughness, shading_normal, view, instance, input.texcoord, ddx(input.texcoord), ddy(input.texcoord), tangent_bits, weather.wetness_, weather.snow_coverage_);
		ConstantBuffer<LightConstantBuffer> constants = ResourceDescriptorHeap[bindings.light_constants_];
		float3 lighting = float3(0.0, 0.0, 0.0);

		if (constants.sun_intensity_ > 0.0)
		{
			float3 sun_direction = normalize(-constants.sun_direction_);
			RayDesc shadow_ray;
			shadow_ray.Origin = input.world_position + normal * 0.01;
			shadow_ray.Direction = sun_direction;
			shadow_ray.TMin = 0.001;
			shadow_ray.TMax = 10000.0;
			RaytracingHitTables tables;
			tables.top_level_ = bindings.top_level_structure_;
			tables.hits_ = bindings.top_level_hits_;
			if (!RaytracingHitIsOccluded(tables, shadow_ray))
			{
				lighting += EvalDirectLightDispatch(instance.shading_.shading_model_, shading_normal, view, sun_direction, material.diffuse_color_, material.f0_, material.roughness_, material.clearcoat_factor_, material.clearcoat_roughness_, material.clearcoat_normal_, material.sheen_color_, material.sheen_roughness_, material.anisotropy_tangent_, material.anisotropy_bitangent_, material.anisotropy_strength_) * constants.sun_color_ * constants.sun_intensity_;
			}
		}

		uint2 screen_size = uint2(scene.screen_size_);
		uint2 pixel = min(uint2(input.position.xy), screen_size - 1);
		float view_depth = mul(float4(input.world_position, 1.0), scene.view_).z;
		uint2 tile_count = (screen_size + LIGHT_CLUSTER_TILE_SIZE - 1) / LIGHT_CLUSTER_TILE_SIZE;
		uint2 tile = min(pixel / LIGHT_CLUSTER_TILE_SIZE, tile_count - 1);
		uint slice = 0;
		if (view_depth >= scene.far_plane_)
		{
			slice = LIGHT_CLUSTER_DEPTH_SLICES - 1;
		}
		else if (view_depth > scene.near_plane_)
		{
			float log_near = log(scene.near_plane_);
			slice = min(uint((log(view_depth) - log_near) / (log(scene.far_plane_) - log_near) * LIGHT_CLUSTER_DEPTH_SLICES), LIGHT_CLUSTER_DEPTH_SLICES - 1);
		}
		uint cluster = (slice * tile_count.y + tile.y) * tile_count.x + tile.x;

		StructuredBuffer<uint> grid = ResourceDescriptorHeap[bindings.light_cluster_grid_];
		StructuredBuffer<LightStructuredBuffer> light_list = ResourceDescriptorHeap[bindings.light_list_];
		uint base = cluster * (1 + LIGHT_CLUSTER_MAX_LIGHTS);
		uint light_count = grid[base];
		for (uint light_slot = 0; light_slot < light_count; light_slot++)
		{
			LightStructuredBuffer light = light_list[grid[base + 1 + light_slot]];

			float3 light_position = light.position_;
			if (light.type_ == LIGHT_TYPE_RECT)
			{
				float3 offset = input.world_position - light.position_;
				if (dot(offset, light.direction_) <= 0.0)
				{
					continue;
				}
				light_position += light.right_ * clamp(dot(offset, light.right_), -light.half_width_, light.half_width_);
				light_position += light.up_ * clamp(dot(offset, light.up_), -light.half_height_, light.half_height_);
			}

			float3 to_light = light_position - input.world_position;
			float distance_to_light = length(to_light);
			float3 light_direction = to_light / max(distance_to_light, 0.0001);

			float attenuation_ratio = saturate(distance_to_light / light.range_);
			float attenuation_ratio2 = attenuation_ratio * attenuation_ratio;
			float attenuation = saturate(1.0 - attenuation_ratio2 * attenuation_ratio2);
			attenuation = attenuation * attenuation / max(distance_to_light * distance_to_light, 0.0001);
			if (light.type_ == LIGHT_TYPE_SPOT)
			{
				float cos_angle = dot(-light_direction, light.direction_);
				attenuation *= saturate((cos_angle - light.cos_half_angle_) / max(light.softness_ * (1.0 - light.cos_half_angle_), 0.0001));
			}
			if (attenuation <= 0.0)
			{
				continue;
			}
			lighting += EvalDirectLightDispatch(instance.shading_.shading_model_, shading_normal, view, light_direction, material.diffuse_color_, material.f0_, material.roughness_, material.clearcoat_factor_, material.clearcoat_roughness_, material.clearcoat_normal_, material.sheen_color_, material.sheen_roughness_, material.anisotropy_tangent_, material.anisotropy_bitangent_, material.anisotropy_strength_) * light.color_ * light.intensity_ * attenuation;
		}

		if (constants.sky_intensity_ > 0.0)
		{
			float occlusion = 1.0;
			if (instance.texture_.occlusion_texture_index_ != MODEL_INVALID)
			{
				Texture2D<float4> occlusion_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.occlusion_texture_index_)];
				occlusion = occlusion_texture.Sample(sampler_linear_wrap, input.texcoord).r;
			}
			lighting += ImageBasedLightingRadianceLambertian(bindings.sky_brdf_lut_, bindings.sky_diffuse_irradiance_, shading_normal, view, material.roughness_, material.diffuse_color_, material.f0_) * occlusion * constants.sky_intensity_;
			lighting += ImageBasedLightingRadianceGgx(bindings.sky_brdf_lut_, bindings.sky_specular_prefiltered_, shading_normal, view, material.roughness_, material.f0_) * occlusion * constants.sky_intensity_;
		}
		radiance = lighting + emissive;
	}

	float alpha = saturate(base_color.a);
	float3 color = radiance * alpha;
	float2 velocity = (input.current_clip.xy / input.current_clip.w - input.previous_clip.xy / input.previous_clip.w) * 0.5;

	StructuredBuffer<OutlineStyleStructuredBuffer> outline_styles = ResourceDescriptorHeap[bindings.outline_styles_];
	if (instance.shading_.actor_ != MODEL_INVALID && outline_styles[instance.shading_.actor_].flags_ != 0)
	{
		RWTexture2D<uint> outline_mask = ResourceDescriptorHeap[bindings.outline_mask_];
		InterlockedMax(outline_mask[uint2(input.position.xy)], (max(uint(saturate(input.position.z) * 32767.0), 1u) << 17) | instance.shading_.actor_);
	}

	RWStructuredBuffer<uint> counter = ResourceDescriptorHeap[bindings.transparent_counter_];
	RWStructuredBuffer<TransparentFragmentStructuredBuffer> fragments = ResourceDescriptorHeap[bindings.transparent_fragments_];
	RWTexture2D<uint> heads = ResourceDescriptorHeap[bindings.transparent_heads_];
	uint capacity;
	uint stride;
	fragments.GetDimensions(capacity, stride);
	uint index;
	InterlockedAdd(counter[0], 1, index);
	if (index >= capacity)
	{
		return;
	}

	uint next;
	InterlockedExchange(heads[uint2(input.position.xy)], index, next);

	TransparentFragmentStructuredBuffer fragment;
	fragment.color_rg_ = f32tof16(color.r) | (f32tof16(color.g) << 16);
	fragment.color_ba_ = f32tof16(color.b) | (f32tof16(alpha) << 16);
	fragment.depth_ = input.position.z;
	fragment.next_ = next;
	fragment.velocity_ = f32tof16(velocity.x) | (f32tof16(velocity.y) << 16);
	fragments[index] = fragment;
}
