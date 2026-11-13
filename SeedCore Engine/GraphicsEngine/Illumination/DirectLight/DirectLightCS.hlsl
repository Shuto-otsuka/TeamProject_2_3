#include <GraphicsEngine/Light/Light.hlsli>
#include <GraphicsEngine/Light/LightCluster.h>
#include <GraphicsEngine/Light/LightShading.hlsli>
#include <GraphicsEngine/Model/Material/Material.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Environment/Weather/Weather.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

struct DirectLightRatioBindings
{
	uint geometry_depth_;
	uint material_surface_base_color_;
	uint material_surface_normal_roughness_;
	uint material_surface_tangent_;
	uint geometry_visibility_id_;
	uint geometry_visibility_texcoord_;
	uint geometry_instances_;
	uint light_list_;
	uint light_constants_;
	uint weather_constants_;
	uint light_cluster_grid_;
	uint raytracing_shadow_visibility_;
	uint direct_light_punctual_radiance_;
};
ConstantBuffer<DirectLightRatioBindings> bindings : register(b0, space1);

/**
* [EN]
* DirectLight.Resolve: one thread per pixel. Rebuilds the surface from
* the G-Buffer and its material (LoadSurfaceMaterial), finds the pixel's
* cluster from its screen tile and its logarithmic depth slice, and sums the
* unshadowed direct light of every point, spot and rectangle light in it. All
* three share one smooth distance falloff to zero at range_; a spot fades
* over softness_ inside its cone, and a rectangle lights only its front side,
* from the point of the rectangle nearest to the surface. The sum is then
* scaled by the punctual visibility ratio in RaytracingShadow.Visibility.y.
* The background receives no light.
*
* ---------------------------------------------------------------------
*
* [JP]
* DirectLight.Resolve。1 ピクセルに 1 スレッド。G-Buffer とマテリアル
* （LoadSurfaceMaterial）から表面を組み立て、画面のタイルと対数の深さの
* スライスからピクセルのクラスタを求め、そこに入る点光源・スポット・矩形
* ライトの影なしの直接光を合計し、RaytracingShadow.Visibility.y の点光源の
* 可視率を掛ける。3 種類とも range_ で 0 になる同じ
* なめらかな距離の減衰を使い、スポットはコーンの内側で softness_ の幅で
* フェードし、矩形は表側だけを、表面にいちばん近い矩形上の点から照らす。
* 背景には光を書かない。
*/
[numthreads(8, 8, 1)]
void Ratio(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<float4> punctual_radiance = ResourceDescriptorHeap[bindings.direct_light_punctual_radiance_];
	Texture2D<float> depth_texture = ResourceDescriptorHeap[bindings.geometry_depth_];
	uint width;
	uint height;
	depth_texture.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	uint2 pixel = id.xy;
	float depth = depth_texture.Load(int3(pixel, 0));
	Texture2D<uint2> visibility_id = ResourceDescriptorHeap[bindings.geometry_visibility_id_];
	if (depth == 0.0 || visibility_id.Load(int3(pixel, 0)).x == 0)
	{
		punctual_radiance[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	float2 uv = (float2(pixel) + 0.5) / float2(width, height);
	float4 world = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), scene.inverse_view_projection_);
	float3 world_position = world.xyz / world.w;
	float3 view = normalize(scene.camera_position_.xyz - world_position);
	float view_depth = mul(float4(world_position, 1.0), scene.view_).z;

	Texture2D<float4> surface_base_color = ResourceDescriptorHeap[bindings.material_surface_base_color_];
	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_surface_normal_roughness_];
	Texture2D<uint> surface_tangent = ResourceDescriptorHeap[bindings.material_surface_tangent_];
	Texture2D<float2> visibility_texcoord = ResourceDescriptorHeap[bindings.geometry_visibility_texcoord_];
	float4 base_color = surface_base_color.Load(int3(pixel, 0));
	uint normal_roughness = surface_normal_roughness.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(float2(normal_roughness & 0xFFF, (normal_roughness >> 12) & 0xFFF) / 4095.0);
	float roughness = (normal_roughness >> 24) / 255.0;

	uint instance_index;
	uint meshlet_index;
	uint triangle_in_meshlet_index;
	UnpackVisibilityID(visibility_id.Load(int3(pixel, 0)), instance_index, meshlet_index, triangle_in_meshlet_index);
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[instance_index];

	uint2 pixel_x = uint2(min(pixel.x + 1, width - 1), pixel.y);
	uint2 pixel_y = uint2(pixel.x, min(pixel.y + 1, height - 1));
	float2 texcoord = visibility_texcoord.Load(int3(pixel, 0));
	float2 texcoord_dx = visibility_id.Load(int3(pixel_x, 0)).x == instance_index + 1 ? visibility_texcoord.Load(int3(pixel_x, 0)) - texcoord : float2(0.0, 0.0);
	float2 texcoord_dy = visibility_id.Load(int3(pixel_y, 0)).x == instance_index + 1 ? visibility_texcoord.Load(int3(pixel_y, 0)) - texcoord : float2(0.0, 0.0);

	ConstantBuffer<WeatherConstantBuffer> weather = ResourceDescriptorHeap[bindings.weather_constants_];
	SurfaceMaterial material = LoadSurfaceMaterial(base_color.rgb, base_color.a, roughness, normal, view, instance, texcoord, texcoord_dx, texcoord_dy, surface_tangent.Load(int3(pixel, 0)), weather.wetness_, weather.snow_coverage_);
	if (scene.view_mode_ == VIEW_MODE_DETAIL_LIGHTING)
	{
		material.diffuse_color_ = float3(0.5, 0.5, 0.5);
		material.f0_ = float3(0.04, 0.04, 0.04);
	}
	else if (scene.view_mode_ == VIEW_MODE_LIGHTING_ONLY)
	{
		material.diffuse_color_ = float3(1.0, 1.0, 1.0);
		material.f0_ = float3(0.04, 0.04, 0.04);
	}
	else if (scene.view_mode_ == VIEW_MODE_DIRECT_DIFFUSE)
	{
		material.f0_ = float3(0.0, 0.0, 0.0);
	}
	else if (scene.view_mode_ == VIEW_MODE_DIRECT_SPECULAR)
	{
		material.diffuse_color_ = float3(0.0, 0.0, 0.0);
	}

	uint2 tile_count = (uint2(width, height) + LIGHT_CLUSTER_TILE_SIZE - 1) / LIGHT_CLUSTER_TILE_SIZE;
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
	uint base = cluster * (1 + LIGHT_CLUSTER_MAX_LIGHTS);
	uint light_count = grid[base];

	float3 lighting = float3(0.0, 0.0, 0.0);
	if (light_count > 0)
	{
		StructuredBuffer<LightStructuredBuffer> lights = ResourceDescriptorHeap[bindings.light_list_];
		for (uint light_slot = 0; light_slot < light_count; light_slot++)
		{
			LightStructuredBuffer light = lights[grid[base + 1 + light_slot]];

			float3 light_position = light.position_;
			if (light.type_ == LIGHT_TYPE_RECT)
			{
				float3 offset = world_position - light.position_;
				if (dot(offset, light.direction_) <= 0.0)
				{
					continue;
				}
				light_position += light.right_ * clamp(dot(offset, light.right_), -light.half_width_, light.half_width_);
				light_position += light.up_ * clamp(dot(offset, light.up_), -light.half_height_, light.half_height_);
			}

			float3 to_light = light_position - world_position;
			float distance_to_light = length(to_light);
			float3 light_direction = to_light / max(distance_to_light, 0.0001);
			if (dot(normal, light_direction) <= 0.0)
			{
				continue;
			}

			float attenuation_ratio = saturate(distance_to_light / light.range_);
			float attenuation_ratio2 = attenuation_ratio * attenuation_ratio;
			float attenuation = saturate(1.0 - attenuation_ratio2 * attenuation_ratio2);
			attenuation = attenuation * attenuation / max(distance_to_light * distance_to_light, 0.0001);
			if (light.type_ == LIGHT_TYPE_SPOT)
			{
				float cos_angle = dot(-light_direction, light.direction_);
				attenuation *= saturate((cos_angle - light.cos_half_angle_) / max(light.softness_ * (1.0 - light.cos_half_angle_), 0.0001));
			}

			float3 light_radiance = light.color_ * light.intensity_ * attenuation;
			if (any(light_radiance > 0.0))
			{
				lighting += EvalDirectLightDispatch(instance.shading_.shading_model_, normal, view, light_direction, material.diffuse_color_, material.f0_, material.roughness_, material.clearcoat_factor_, material.clearcoat_roughness_, material.clearcoat_normal_, material.sheen_color_, material.sheen_roughness_, material.anisotropy_tangent_, material.anisotropy_bitangent_, material.anisotropy_strength_) * light_radiance;
			}
		}
	}
	Texture2D<float2> shadow_visibility = ResourceDescriptorHeap[bindings.raytracing_shadow_visibility_];
	punctual_radiance[pixel] = float4(lighting * shadow_visibility.Load(int3(pixel, 0)).y, 0.0);
}
