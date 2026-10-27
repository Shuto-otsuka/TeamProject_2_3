#include "../../../Shader/Scene.hlsli"
#include "../../../Shader/ShaderResources.hlsli"
#include "../../../Shader/UnorderedAccesses.hlsli"
#include "../../../Shader/Constants.hlsli"
#include "../../../Light/Light.hlsli"
#include "../../../Shader/Normal.hlsli"
#include "../../../Shader/Noise.hlsli"
#include "../../../Shader/Material.hlsli"
#include "../../../Light/Cluster.hlsli"
#include "../../Reflection/Raytracing/RaytracingReflection.hlsli"
#include "RaytracingShadow.hlsli"

/**
* [EN]
* Reference:
* - https://cs.dartmouth.edu/~wjarosz/publications/bitterli20spatiotemporal.html
*   (Bitterli et al., "Spatiotemporal reservoir resampling for real-time ray
*   tracing with dynamic direct lighting", SIGGRAPH 2020 - the RIS/ReSTIR DI
*   selection scheme this pass's punctual-light picker follows, minus its
*   temporal/spatial reuse passes, which are not implemented here.)
* - https://intro-to-restir.cwyman.org/presentations/2023ReSTIR_Course_Notes.pdf
*   (Wyman et al., ReSTIR course notes - target function / RIS weighting
*   background.)
*
* Stochastic ray-traced shadow + ReSTIR DI pass (inline RayQuery). One thread
* per screen pixel, ray_count_ shadow rays per light per pixel per frame:
*   - Directional light: always tested (cheap, single dominant light), each ray
*     jittered within a cone sized by sun_angular_radius_ for a soft penumbra.
*     Output stays a scalar visibility, same as before.
*   - Point/Spot/Rect lights: picks ONE light per pixel per frame via
*     weighted reservoir sampling (no per-light cost growth) — the weight is
*     each light's actual contribution to this pixel (intensity * distance
*     attenuation * spot cone fade, zero if behind the surface, outside the
*     spot cone, or the wrong side of a rect light), so lights that don't
*     actually illuminate the pixel are never selected and never waste the
*     shadow rays. Jitters each ray toward a random point on/near the
*     picked light's surface (physical size -> free soft shadow for rect
*     lights; a fixed world-space radius for point/spot). Unlike the
*     directional term, this is a full ReSTIR DI-style estimator: it resolves
*     this pixel's G-Buffer material (Shader/Material.hlsli's
*     ResolveGBufferMaterial - the SAME function Model/Opaque/
*     DeferredLightingPS.hlsl uses, so the specular response matches),
*     evaluates the picked light's full BRDF response, divides by the RIS
*     selection pdf (picked_weight / weight_sum) to keep the single-sample
*     estimate unbiased, and multiplies by the shadow rays' averaged visibility. The
*     result is a noisy single-sample RGB radiance estimate rather than a
*     scalar - RaytracingShadowDenoiseCS.hlsl accumulates it over time (tracking
*     variance from its luminance, since a full RGB covariance would triple
*     the moment storage for no denoising benefit). This replaces sharing one
*     averaged visibility scalar across every punctual light (like UE Lumen's
*     stochastic shadowing) - dozens of lights can affect one pixel, so tracing
*     one full shadow ray per light per frame does not scale, but averaging
*     their visibility together was wrong: a light that is actually occluded
*     would only partially darken, blended with whichever other light in the
*     cluster happened to be lit.
*
* Output: raw_visibility_uav_index_, r = directional visibility, gba = the
* picked punctual light's RGB radiance estimate, both raw/noisy.
*
* ---------------------------------------------------------------------
*
* [JP]
* 確率的レイトレ影 + ReSTIR DI パス(インライン RayQuery)。1スレッド=1
* 画面ピクセル、1フレームにつき光源ごとに影レイを ray_count_ 本撃つ:
*   - ディレクショナルライト: 常に評価する(主光源1灯だけなので安い)。
*     sun_angular_radius_ で決まるコーン内でレイをジッターしてソフトな
*     半影を作る。出力は従来通りスカラー可視性のまま。
*   - Point/Spot/Rect ライト: このピクセルを照らす候補から重み付き
*     リザーバーサンプリングで毎フレーム【1灯だけ】選ぶ(灯数が増えても
*     コストが増えない) - 重みは各光の実効寄与(強度×距離減衰×スポット
*     フェード、面に対して背面/スポット圏外/矩形の裏側なら0)なので、この
*     ピクセルを実際には照らしていない光が選ばれて影レイを無駄にすることは
*     ない。選ばれた光の表面上/近傍のランダムな点へ各レイをジッターする
*     (物理サイズがそのまま矩形光のソフトシャドウになる。Point/Spot は
*     固定のワールド空間半径)。ディレクショナル項と違い、こちらは完全な
*     ReSTIR DI 型の推定量: このピクセルの G-Buffer マテリアルを解決し
*     (Shader/Material.hlsli の ResolveGBufferMaterial - Model/Opaque/
*     DeferredLightingPS.hlsl と【同じ】関数を使うので鏡面ハイライトの
*     形が一致する)、選ばれた光の完全な BRDF 応答を評価し、RIS の選択
*     pdf(picked_weight / weight_sum)で割って単一サンプル推定量を
*     アンバイアスに保ち、影レイの平均可視性を掛ける。結果はスカラーではなく
*     ノイズを含む単一サンプルの RGB 放射輝度推定量になる -
*     RaytracingShadowDenoiseCS.hlsl がこれを時間積分する(モーメントは輝度から追跡
*     する - RGB 共分散をフルで持つとモーメントの保存量が3倍になり、
*     デノイズの得にはならないため)。これは、UE Lumen の確率的シャドウの
*     ように可視性スカラー1つを全パンクチュアルライトで共有する方式を
*     置き換える - 1ピクセルに何十灯も影響し得るので毎フレーム全灯に
*     フルの影レイを撃つのはスケールしないが、可視性を平均するのは誤り
*     だった: 実際に遮蔽されている光は部分的にしか暗くならず、クラスタ内の
*     たまたま照射中の別の光と混ざってしまう。
*
* 出力: raw_visibility_uav_index_、r = ディレクショナル可視性、gba = 選ばれた
* パンクチュアルライトの RGB 放射輝度推定量、どちらも生値/ノイズ入り。
*/
[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	if (dtid.x >= (uint)scene.screen_size_.x || dtid.y >= (uint)scene.screen_size_.y)
	{
		return;
	}

	uint2 pixel = dtid.xy;
	RWTexture2D<float4> raw_signal = ResourceDescriptorHeap[unordered_access_indices.shadow_.raw_visibility_index_];

	Texture2D<float> depth_texture = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.depth_index_];
	float depth = depth_texture.Load(int3(pixel, 0));

	if (depth == 0.0)
	{
		raw_signal[pixel] = float4(1.0, 0.0, 0.0, 0.0);
		return;
	}

	float2 uv = (float2(pixel) + 0.5) * scene.inverse_screen_size_;
	float2 ndc = float2(uv.x * 2 - 1, 1 - uv.y * 2);
	float4 clip = float4(ndc, depth, 1.0);
	float4 world = mul(clip, scene.inverse_view_projection_);
	float3 world_position = world.xyz / world.w;

	ConstantBuffer<RaytracingShadowConstantBuffer> tuning = ResourceDescriptorHeap[constant_indices.shadow_index_];

	Texture2D<float4> normal_texture = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.index_1_];
	float4 rt1 = normal_texture.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(rt1.rg);
	float3 origin = world_position + normal * tuning.normal_bias_;

	RaytracingAccelerationStructure tlas = ResourceDescriptorHeap[shader_resource_indices.raytracing_.tlas_index_];

	uint rng_state = SeedFromPixel(pixel, tuning.frame_index_);

	float directional_visibility = 1.0;
	ConstantBuffer<LightConstantBuffer> light = ResourceDescriptorHeap[constant_indices.light_index_];
	float3 directional_base = normalize(-GetDirectionalLightConstantBuffer().direction_);

	if (GetDirectionalLightConstantBuffer().sun_intensity_ > 0.0)
	{
		if (dot(normal, directional_base) <= 0.0)
		{
			directional_visibility = 0.0;
		}
		else
		{
			float cos_theta_max = cos(tuning.sun_angular_radius_);
			float visible_count = 0.0;

			for (uint index = 0; index < tuning.ray_count_; index++)
			{
				float3 ray_direction = SampleCone(rng_state, directional_base, cos_theta_max);

				RayDesc ray_desc;
				ray_desc.Origin = origin;
				ray_desc.Direction = ray_direction;
				ray_desc.TMin = 0.001;
				ray_desc.TMax = tuning.ray_t_max_;

                visible_count += IsRayOccluded(tlas, ray_desc, shader_resource_indices.raytracing_.instance_data_index_) ? 0.0 : 1.0;
            }

			directional_visibility = visible_count / (float)tuning.ray_count_;
		}
	}

	float3 punctual_radiance = float3(0, 0, 0);

	float4 view_position = mul(float4(world_position, 1.0), scene.view_);
	float linear_depth = view_position.z;

	uint3 cluster_count = ComputeClusterCount(scene.screen_size_);
	uint tile_x = pixel.x / CLUSTER_TILE_SIZE;
	uint tile_y = pixel.y / CLUSTER_TILE_SIZE;
	uint slice = ComputeDepthSlice(linear_depth, scene.near_plane_, scene.far_plane_);
	uint cluster_index = ClusterIndex(uint3(tile_x, tile_y, slice), cluster_count);

	StructuredBuffer<ClusterInstance> cluster_data = ResourceDescriptorHeap[shader_resource_indices.light_.cluster_data_index_];
	ClusterInstance cluster = cluster_data[cluster_index];

	uint point_count = min(cluster.point_count_, CLUSTER_MAX_POINT_LIGHTS);
	uint spot_count = min(cluster.spot_count_, CLUSTER_MAX_SPOT_LIGHTS);
	uint rect_count = min(cluster.rect_count_, CLUSTER_MAX_RECT_LIGHTS);
	uint total_punctual = point_count + spot_count + rect_count;

	if (total_punctual > 0)
	{
		ByteAddressBuffer light_list = ResourceDescriptorHeap[shader_resource_indices.light_.cluster_light_list_index_];
		uint base = cluster_index * CLUSTER_STRIDE;

		StructuredBuffer<PointLightStructuredBuffer> point_lights = GetPointLightStructuredBuffer(shader_resource_indices.light_.point_light_index_);
		StructuredBuffer<SpotLightStructuredBuffer> spot_lights = GetSpotLightStructuredBuffer(shader_resource_indices.light_.spot_light_index_);
		StructuredBuffer<RectLightStructuredBuffer> rect_lights = GetRectLightStructuredBuffer(shader_resource_indices.light_.rect_light_index_);

		float weight_sum = 0.0;
		float picked_weight = 0.0;
		uint picked = 0xFFFFFFFF;

		for (uint index = 0; index < total_punctual; index++)
		{
			float weight = 0.0;

			if (index < point_count)
			{
				uint light_index = light_list.Load((base + index) * 4);
				PointLightStructuredBuffer point_light = point_lights[light_index];

				float3 to_light = point_light.position_ - world_position;
				float distance_to_light = length(to_light);
				float3 light_direction = to_light / max(distance_to_light, 0.0001);

				if (dot(normal, light_direction) > 0.0)
				{
					float attenuation_ratio = saturate(distance_to_light / point_light.range_);
					float attenuation_ratio2 = attenuation_ratio * attenuation_ratio;
					float attenuation = saturate(1.0 - attenuation_ratio2 * attenuation_ratio2);
					attenuation = attenuation * attenuation / max(distance_to_light * distance_to_light, 0.0001);
					weight = point_light.intensity_ * attenuation;
				}
			}
			else if (index < point_count + spot_count)
			{
				uint local_index = index - point_count;
				uint light_index = light_list.Load((base + CLUSTER_MAX_POINT_LIGHTS + local_index) * 4);
				SpotLightStructuredBuffer spot_light = spot_lights[light_index];

				float3 to_light = spot_light.position_ - world_position;
				float distance_to_light = length(to_light);
				float3 light_direction = to_light / max(distance_to_light, 0.0001);

				if (dot(normal, light_direction) > 0.0)
				{
					float cos_angle = dot(-light_direction, spot_light.direction_);
					float spot_fade = saturate((cos_angle - spot_light.cos_half_angle_) / max(spot_light.softness_ * (1.0 - spot_light.cos_half_angle_), 0.0001));
					float attenuation_ratio = saturate(distance_to_light / spot_light.range_);
					float attenuation_ratio2 = attenuation_ratio * attenuation_ratio;
					float attenuation = saturate(1.0 - attenuation_ratio2 * attenuation_ratio2);
					attenuation = attenuation * attenuation / max(distance_to_light * distance_to_light, 0.0001);
					weight = spot_light.intensity_ * attenuation * spot_fade;
				}
			}
			else
			{
				uint local_index = index - point_count - spot_count;
				uint light_index = light_list.Load((base + CLUSTER_MAX_POINT_LIGHTS + CLUSTER_MAX_SPOT_LIGHTS + local_index) * 4);
				RectLightStructuredBuffer rect_light = rect_lights[light_index];

				if (dot(world_position - rect_light.position_, rect_light.normal_) > 0.0)
				{
					float3 rect_delta = world_position - rect_light.position_;
					float rect_local_x = clamp(dot(rect_delta, rect_light.right_), -rect_light.half_width_, rect_light.half_width_);
					float rect_local_y = clamp(dot(rect_delta, rect_light.up_), -rect_light.half_height_, rect_light.half_height_);
					float3 representative_point = rect_light.position_ + rect_light.right_ * rect_local_x + rect_light.up_ * rect_local_y;
					float3 to_light = representative_point - world_position;
					float distance_to_light = length(to_light);
					float3 light_direction = to_light / max(distance_to_light, 0.0001);

					if (dot(normal, light_direction) > 0.0)
					{
						float attenuation_ratio = saturate(distance_to_light / rect_light.range_);
						float attenuation_ratio2 = attenuation_ratio * attenuation_ratio;
						float attenuation = saturate(1.0 - attenuation_ratio2 * attenuation_ratio2);
						attenuation = attenuation * attenuation / max(distance_to_light * distance_to_light, 0.0001);
						weight = rect_light.intensity_ * attenuation;
					}
				}
			}

			if (weight > 0.0)
			{
				weight_sum += weight;

				if (Rand(rng_state) * weight_sum <= weight)
				{
					picked = index;
					picked_weight = weight;
				}
			}
		}

		if (picked >= total_punctual)
		{
			weight_sum = 0.0;
		}

		if (weight_sum > 0.0)
		{
			float3 sample_point;
			float3 light_color;
			float3 base_direction;
			float light_radius = tuning.punctual_light_radius_;
            float visible_count = 0.0;

            for (uint ray_index = 0; ray_index < tuning.ray_count_; ray_index++)
            {
                if (picked < point_count)
                {
                    uint light_index = light_list.Load((base + picked) * 4);
                    PointLightStructuredBuffer point_light = point_lights[light_index];
                    sample_point = point_light.position_;
                    light_color = point_light.color_.rgb;
                }
                else if (picked < point_count + spot_count)
                {
                    uint local_index = picked - point_count;
                    uint light_index = light_list.Load((base + CLUSTER_MAX_POINT_LIGHTS + local_index) * 4);
                    SpotLightStructuredBuffer spot_light = spot_lights[light_index];
                    sample_point = spot_light.position_;
                    light_color = spot_light.color_.rgb;
                }
                else
                {
                    uint local_index = picked - point_count - spot_count;
                    uint light_index = light_list.Load((base + CLUSTER_MAX_POINT_LIGHTS + CLUSTER_MAX_SPOT_LIGHTS + local_index) * 4);
                    RectLightStructuredBuffer rect_light = rect_lights[light_index];
                    float2 rect_uv = Rand2(rng_state) * 2.0 - 1.0;
                    sample_point = rect_light.position_ + rect_light.right_ * (rect_uv.x * rect_light.half_width_) + rect_light.up_ * (rect_uv.y * rect_light.half_height_);
                    light_color = rect_light.color_.rgb;
                    light_radius = 0.0;
                }

                float3 to_light = sample_point - world_position;
                float distance_to_light = length(to_light);
                base_direction = to_light / max(distance_to_light, 0.0001);

                float sin_theta_max = saturate(light_radius / max(distance_to_light, 0.001));
                float cos_theta_max = sqrt(max(0.0, 1.0 - sin_theta_max * sin_theta_max));
                float3 ray_direction = (light_radius > 0.0) ? SampleCone(rng_state, base_direction, cos_theta_max) : base_direction;

                RayDesc ray_desc;
                ray_desc.Origin = origin;
                ray_desc.Direction = ray_direction;
                ray_desc.TMin = 0.001;
                ray_desc.TMax = max(distance_to_light - 0.01, 0.001);
				
                visible_count += IsRayOccluded(tlas, ray_desc, shader_resource_indices.raytracing_.instance_data_index_) ? 0.0 : 1.0;
            }
			
            float ray_visibility = visible_count / (float)tuning.ray_count_;
			float shadow_factor = saturate(lerp(1.0, ray_visibility, max(tuning.shadow_strength_, 0.0)));

			Texture2D<float4> gbuffer0 = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.index_0_];
			float4 rt0 = gbuffer0.Load(int3(pixel, 0));
			float3 base_color = rt0.rgb;
			float metallic = rt0.a;
			float roughness = rt1.b;

			Texture2D<uint4> gbuffer4 = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.index_4_];
			uint4 visibility_id = gbuffer4.Load(int3(pixel, 0));
			uint material_instance_index, material_meshlet_index, material_triangle_index;
			UnpackVisibilityID(visibility_id, material_instance_index, material_meshlet_index, material_triangle_index);
			StructuredBuffer<ModelStructuredBuffer> material_instances = GetModelStructuredBuffer(shader_resource_indices.model_.instance_index_);
			ModelStructuredBuffer material_instance = material_instances[material_instance_index];

			float3 view = normalize(scene.camera_position_.xyz - world_position);
			float2 material_texcoord = float2(asfloat(visibility_id.z), asfloat(visibility_id.w));
			GBufferMaterial gbuffer_material = ResolveGBufferMaterial(base_color, metallic, roughness, normal, view, material_instance, material_texcoord, rt1.a);

			float3 brdf_response = EvalDirectLightDispatch(material_instance.shading_.shading_model_, normal, view, base_direction, gbuffer_material.diffuse_color_, gbuffer_material.f0_, gbuffer_material.roughness_, gbuffer_material.clearcoat_factor_, gbuffer_material.clearcoat_roughness_, gbuffer_material.clearcoat_normal_, gbuffer_material.sheen_color_, gbuffer_material.sheen_roughness_, gbuffer_material.anisotropy_tangent_, gbuffer_material.anisotropy_bitangent_, gbuffer_material.anisotropy_strength_);

			punctual_radiance = brdf_response * light_color * weight_sum * shadow_factor;
		}
	}

	raw_signal[pixel] = float4(directional_visibility, punctual_radiance);
}
