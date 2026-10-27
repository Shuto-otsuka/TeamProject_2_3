#include "RaytracingReflection.hlsli"
#include "RaytracingReflectionReSTIR.hlsli"
#include "../../../Shader/Scene.hlsli"
#include "../../../Shader/ShaderResources.hlsli"
#include "../../../Shader/Constants.hlsli"
#include "../../../Light/Light.hlsli"
#include "../../../Shader/Normal.hlsli"
#include "../../../Shader/Noise.hlsli"
#include "../../../Shader/Denoiser.hlsli"
#include "../../../Shader/UnorderedAccesses.hlsli"
#include "../../../Shader/Vertex.hlsli"
#include "../../../Sky/IBL/ImageBasedLighting.hlsli"
#include "../../../Environment/VolumetricCloud/VolumetricCloudScapes.hlsli"
#include "../../../Sky/Sky.hlsli"

/**
* [EN]
* Reference:
* - https://jcgt.org/published/0007/04/01/paper.pdf
*   (Heitz, "Sampling the GGX Distribution of Visible Normals", JCGT 2018 -
*   the visible-normal-distribution sampling this raygen uses instead of the
*   older full-NDF ImportanceSampleGgx.)
* - https://arxiv.org/pdf/2306.05044
*   (Dupuy & Benyoub, "Sampling Visible GGX Normals with Spherical Caps",
*   HPG 2023 - the exact spherical-cap construction SampleGgxVisibleNormal
*   implements, Sky.hlsli.)
* - https://cs.dartmouth.edu/~wjarosz/publications/bitterli20spatiotemporal.html
*   (Bitterli et al., "Spatiotemporal reservoir resampling for real-time ray
*   tracing with dynamic direct lighting", SIGGRAPH 2020 - the reservoir
*   reuse this raygen and the following spatial pass are built on.)
*
* Ray-traced glossy reflection (RTPSO / DispatchRays, raygen + miss +
* closesthit). Reflects the view ray off the G-Buffer surface and traces
* ray_count_ rays per pixel per frame (one on a near-mirror surface, where every
* sample would land on the same direction), each half vector drawn from the GGX distribution of
* VISIBLE normals (Sky.hlsli's SampleGgxVisibleNormal) rather than the
* full NDF - only microfacets the viewer can actually see are sampled, so the
* single-sample estimator needs no pdf division or cosine weight: its whole
* weight is the height-correlated Smith masking-shadowing ratio G2/G1
* (Sky.hlsli's SmithGgxG2OverG1), applied to the traced radiance below.
* A sample landing below the horizon (G2/G1 == 0, which VNDF sampling makes
* rare but not impossible near grazing angles) is simply not traced and
* contributes nothing - unlike the old full-NDF scheme, there is no need to
* redirect it to the mirror direction as a biased fallback. At roughness 0
* the half-vector always equals the normal, so this degenerates to the exact
* mirror ray with no noise. Each ray becomes one candidate streamed into a
* ReSTIR reservoir (RaytracingReflectionReSTIR.hlsli), which is then combined
* with the reprojected temporal history in the same dispatch and with a few
* spatial neighbors in a following pass (RaytracingReflectionReservoirSpatialCS.hlsl),
* the same scheme RaytracingGlobalIlluminationRT.hlsl uses for GI. The resolved reservoir's
* hit distance is written into the output alpha (not discarded): the
* denoiser's dual reprojection (surface motion + hit-point virtual motion)
* reprojects a virtual position built by extending this hit distance along
* the surface's specular dominant direction. miss samples the environment
* cube; closesthit re-fetches the hit triangle's vertices through the
* per-instance table (InstanceID() -> ReflectionInstanceData) and relights
* the hit point with the directional/punctual lights + diffuse IBL.
*
* ---------------------------------------------------------------------
*
* [JP]
* レイトレ光沢反射(RTPSO / DispatchRays、raygen + miss + closesthit)。
* G-Buffer の面で視線を反射し、1ピクセル1フレームにつきレイを ray_count_ 本
* 撃つ(ほぼ鏡面の面では全サンプルが同じ方向になるので1本)。
* ハーフベクトルは GGX の【可視】法線分布(Sky.hlsli の
* SampleGgxVisibleNormal)からサンプルする - 分布全体ではなく、視線から
* 実際に見えているマイクロファセットだけをサンプルするので、単一サンプル
* 推定量に pdf 除算もコサイン重みも要らない: 重みの全ては高さ相関する
* Smith のマスキング/シャドウイング比 G2/G1(Sky.hlsli の
* SmithGgxG2OverG1)だけで、下でトレース済み放射輝度に掛ける。地平線の下に
* 落ちたサンプル(G2/G1 == 0、VNDF サンプリングでは稀だがグレージング角
* 付近ではあり得る)はそもそもトレースせず寄与ゼロとする - 旧来の分布全体
* サンプリングと違い、これをミラー方向へ差し替えるバイアスありの
* フォールバックは要らない。roughness 0 では常にハーフベクトルが法線と
* 一致するため、ノイズ無しの厳密ミラーレイへ縮退する。各レイは1候補として
* ReSTIR reservoir(RaytracingReflectionReSTIR.hlsli)へストリーム結合され、
* 同じディスパッチ内でリプロジェクション済みの時間的履歴と、続くパス
* (RaytracingReflectionReservoirSpatialCS.hlsl)で近傍数点と結合される。
* RaytracingGlobalIlluminationRT.hlsl が GI で使うのと同じ方式。
* 解決した reservoir のヒット距離は出力アルファへ書く(捨て
* ない) - デノイザの二重リプロジェクション(面モーション+ヒット点仮想
* モーション)がこのヒット距離を面のスペキュラ支配方向へ延長した仮想位置を
* リプロジェクションする。miss は環境キューブをサンプル。closesthit は
* インスタンステーブル(InstanceID() → ReflectionInstanceData)経由で
* ヒット三角形の頂点を引き直し、ディレクショナル/ポイント/スポット/矩形
* ライト+拡散IBLでヒット点を再ライティングする。
*/

/// radiance(12) + hit_distance(4) = 16 bytes, fits the default
/// maxPayloadSizeInBytes (16).
struct RaytracingReflectionPayload
{
	float3 radiance_;
	float hit_distance_;
};

[shader("raygeneration")]
void ReflectionRayGeneration()
{
	uint2 pixel = DispatchRaysIndex().xy;
	RWTexture2D<float4> output = ResourceDescriptorHeap[unordered_access_indices.reflection_.output_index_];

	Texture2D<float> depth_texture = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.depth_index_];
	float depth = depth_texture.Load(int3(pixel, 0));

	if (depth == 0.0)
	{
		output[pixel] = float4(0, 0, 0, 0);
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<RaytracingReflectionConstantBuffer> tuning = ResourceDescriptorHeap[constant_indices.reflection_index_];

	float2 uv = (float2(pixel) + 0.5) * scene.inverse_screen_size_;
	float2 ndc = float2(uv.x * 2 - 1, 1 - uv.y * 2);
	float4 clip = float4(ndc, depth, 1.0);
	float4 world = mul(clip, scene.inverse_view_projection_);
	float3 world_position = world.xyz / world.w;

	Texture2D<float4> normal_texture = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.index_1_];
	float4 gbuffer1 = normal_texture.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(gbuffer1.rg);
	float roughness = gbuffer1.b;

	float3 view_direction = normalize(world_position - scene.camera_position_.xyz);
	float3 mirror_direction = normalize(reflect(view_direction, normal));

	uint rng_state = SeedFromPixel(pixel, tuning.frame_index_ + 2654435761u);
	float3 view = -view_direction;

	RaytracingAccelerationStructure tlas = ResourceDescriptorHeap[shader_resource_indices.raytracing_.tlas_index_];

	float normal_dot_view = saturate(dot(normal, view));
	uint ray_count = roughness < 0.01 ? 1 : tuning.ray_count_;

	RaytracingReflectionReservoir reservoir = (RaytracingReflectionReservoir)0;

	for (uint ray_index = 0; ray_index < ray_count; ray_index++)
	{
		float3 half_vector = SampleGgxVisibleNormal(Rand2(rng_state), normal, view, roughness);
		float3 reflect_direction = normalize(2.0 * dot(view, half_vector) * half_vector - view);

		float normal_dot_light = dot(normal, reflect_direction);
		float sample_weight = SmithGgxG2OverG1(normal_dot_view, normal_dot_light, roughness);

		RaytracingReflectionPayload payload;
		payload.radiance_ = float3(0, 0, 0);
		payload.hit_distance_ = 0.0;

		if (sample_weight > 0.0)
		{
			RayDesc ray_desc;
			ray_desc.Origin = world_position + normal * tuning.normal_bias_;
			ray_desc.Direction = reflect_direction;
			ray_desc.TMin = 0.001;
			ray_desc.TMax = tuning.ray_t_max_;

			TraceRay(tlas, RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES, 0xFF, 0, 0, 0, ray_desc, payload);

			payload.radiance_ *= sample_weight;
		}
		else
		{
			reflect_direction = mirror_direction;
		}

		float3 candidate_radiance = DenoiserSanitizeRadiance(payload.radiance_);
		float candidate_hit_distance = (isnan(payload.hit_distance_) || isinf(payload.hit_distance_)) ? 0.0 : clamp(payload.hit_distance_, 0.0, 65504.0);

		RaytracingReflectionReservoir candidate = ReflectionReservoirFromSample(reflect_direction, candidate_radiance, candidate_hit_distance);

		if (ray_index == 0)
		{
			reservoir = candidate;
		}
		else
		{
			reservoir = ReflectionReservoirCombine(reservoir, candidate, true, rng_state);
		}
	}

	reservoir.receiver_normal_ = normal;
	reservoir.receiver_depth_ = abs(mul(float4(world_position, 1.0), scene.non_jitter_view_projection_).w);

	StructuredBuffer<RaytracingReflectionReservoir> reservoir_history = ResourceDescriptorHeap[shader_resource_indices.reflection_accumulation_.reservoir_history_index_];

	Texture2D<float2> velocity_texture = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.index_2_];
	float2 velocity = velocity_texture.Load(int3(pixel, 0)).rg;
	float2 previous_uv = uv - float2(velocity.x, -velocity.y);

	bool temporal_valid = tuning.temporal_reuse_enabled_ != 0 && all(previous_uv >= 0.0) && all(previous_uv <= 1.0);

	if (temporal_valid)
	{
		uint2 previous_pixel = uint2(previous_uv * scene.screen_size_);
		uint previous_index = previous_pixel.y * (uint)scene.screen_size_.x + previous_pixel.x;

		RaytracingReflectionReservoir temporal = reservoir_history[previous_index];
		temporal.sample_m_ = min(temporal.sample_m_, REFLECTION_RESERVOIR_M_CAP * saturate(roughness / REFLECTION_RESERVOIR_M_CAP_ROUGHNESS));
		temporal.sample_age_ += 1.0;

		float expected_previous_depth = abs(mul(float4(world_position, 1.0), scene.previous_non_jitter_view_projection_).w);
		bool history_valid = temporal.receiver_depth_ > 0.0 && abs(temporal.receiver_depth_ - expected_previous_depth) <= REFLECTION_RESERVOIR_DEPTH_THRESHOLD * expected_previous_depth && dot(normal, temporal.receiver_normal_) >= REFLECTION_RESERVOIR_NORMAL_THRESHOLD && temporal.sample_age_ <= REFLECTION_RESERVOIR_MAX_AGE;

		reservoir = ReflectionReservoirCombine(reservoir, temporal, history_valid, rng_state);
	}

	RWStructuredBuffer<RaytracingReflectionReservoir> reservoir_write = ResourceDescriptorHeap[unordered_access_indices.reflection_accumulation_.reservoir_index_];
	reservoir_write[pixel.y * (uint)scene.screen_size_.x + pixel.x] = reservoir;

	output[pixel] = float4(reservoir.sample_radiance_ * reservoir.sample_w_, reservoir.sample_hit_distance_);
}

[shader("miss")]
void ReflectionMiss(inout RaytracingReflectionPayload payload)
{
	if (shader_resource_indices.sky_.environment_cube_index_ != 0)
	{
		payload.radiance_ = SampleSkyboxEnvironment(WorldRayDirection()).rgb;
	}
	else
	{
		ConstantBuffer<VolumetricCloudScapesRayConstantBuffer> cloud_tuning = ResourceDescriptorHeap[constant_indices.cloud_index_];
		if (cloud_tuning.procedural_sky_enabled_ != 0 && shader_resource_indices.sky_.specular_prefiltered_index_ != 0)
		{
			TextureCube<float4> prefiltered = ResourceDescriptorHeap[shader_resource_indices.sky_.specular_prefiltered_index_];
			payload.radiance_ = prefiltered.SampleLevel(sampler_linear_clamp, WorldRayDirection(), 0).rgb;
		}
		else if (cloud_tuning.procedural_sky_enabled_ != 0)
		{
			ConstantBuffer<LightConstantBuffer> light = ResourceDescriptorHeap[constant_indices.light_index_];
			float3 sun_direction = normalize(-GetDirectionalLightConstantBuffer().direction_);
			float3 sun_radiance = GetDirectionalLightConstantBuffer().sun_color_.rgb * GetDirectionalLightConstantBuffer().sun_intensity_;
			payload.radiance_ = ProceduralSkyColor(WorldRayDirection(), sun_direction, sun_radiance, cloud_tuning);
		}
		else
		{
			payload.radiance_ = float3(0, 0, 0);
		}
	}
	payload.hit_distance_ = 100000.0;
}

[shader("anyhit")]
void ReflectionAnyHit(inout RaytracingReflectionPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
	if (IsMaterialPassthrough(shader_resource_indices.raytracing_.instance_data_index_, InstanceID(), PrimitiveIndex(), attributes.barycentrics))
	{
		IgnoreHit();
	}
}

[shader("closesthit")]
void ReflectionClosestHit(inout RaytracingReflectionPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
	StructuredBuffer<ReflectionInstanceData> instances = ResourceDescriptorHeap[shader_resource_indices.raytracing_.instance_data_index_];
	ReflectionInstanceData instance = instances[InstanceID()];

	StructuredBuffer<uint> triangle_indices = ResourceDescriptorHeap[instance.index_buffer_index_];
	StructuredBuffer<CompressedVertex> vertices = ResourceDescriptorHeap[instance.vertex_buffer_index_];

	uint base_index = PrimitiveIndex() * 3;
	CompressedVertex vertex0 = vertices[triangle_indices[base_index + 0]];
	CompressedVertex vertex1 = vertices[triangle_indices[base_index + 1]];
    CompressedVertex vertex2 = vertices[triangle_indices[base_index + 2]];

	float2 barycentrics = attributes.barycentrics;
	float weight0 = 1.0 - barycentrics.x - barycentrics.y;
	float weight1 = barycentrics.x;
	float weight2 = barycentrics.y;

    float3 object_normal = DecodeCompressedVertexNormal(vertex0) * weight0 + DecodeCompressedVertexNormal(vertex1) * weight1 + DecodeCompressedVertexNormal(vertex2) * weight2;
    float2 texcoord = DecodeCompressedVertexTexcoord(vertex0, instance.texcoord_min_, instance.texcoord_extent_) * weight0 + DecodeCompressedVertexTexcoord(vertex1, instance.texcoord_min_, instance.texcoord_extent_) * weight1 + DecodeCompressedVertexTexcoord(vertex2, instance.texcoord_min_, instance.texcoord_extent_) * weight2;

	float3 world_normal = normalize(mul((float3x3)ObjectToWorld3x4(), object_normal));

	float3 hit_position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<LightConstantBuffer> light = ResourceDescriptorHeap[constant_indices.light_index_];

	float3 lighting = float3(0, 0, 0);

	if (shader_resource_indices.sky_.diffuse_irradiance_index_ != 0)
	{
		TextureCube<float4> diffuse_irradiance = ResourceDescriptorHeap[shader_resource_indices.sky_.diffuse_irradiance_index_];
		lighting += diffuse_irradiance.SampleLevel(sampler_linear_clamp, world_normal, 0).rgb;
	}

	if (GetDirectionalLightConstantBuffer().sun_intensity_ > 0.0)
	{
		float3 light_direction = normalize(-GetDirectionalLightConstantBuffer().direction_);
		float normal_dot_light = saturate(dot(world_normal, light_direction));

		float sun_visibility = 1.0;

		if (normal_dot_light > 0.0)
		{
			ConstantBuffer<RaytracingReflectionConstantBuffer> tuning = ResourceDescriptorHeap[constant_indices.reflection_index_];
			RaytracingAccelerationStructure tlas = ResourceDescriptorHeap[shader_resource_indices.raytracing_.tlas_index_];

			RayDesc shadow_ray;
			shadow_ray.Origin = hit_position + world_normal * tuning.normal_bias_;
			shadow_ray.Direction = light_direction;
			shadow_ray.TMin = 0.001;
			shadow_ray.TMax = tuning.ray_t_max_;

			if (IsRayOccluded(tlas, shadow_ray, shader_resource_indices.raytracing_.instance_data_index_))
			{
				sun_visibility = 0.0;
			}
		}

		const float lambert_normalization = 1.0 / 3.14159265358979;
		lighting += GetDirectionalLightConstantBuffer().sun_color_.rgb * GetDirectionalLightConstantBuffer().sun_intensity_ * normal_dot_light * sun_visibility * lambert_normalization;
	}

	lighting += ComputeClusteredPunctualLighting(hit_position, world_normal, scene, light);

	ReflectionMaterial material = ResolveReflectionMaterial(instance, PrimitiveIndex());

	float3 albedo = material.base_color_;

	if (material.base_color_texture_index_ != 0xFFFFFFFF)
	{
		Texture2D<float4> base_color_texture = ResourceDescriptorHeap[material.base_color_texture_index_];
		albedo *= base_color_texture.SampleLevel(sampler_linear_wrap, texcoord, 0).rgb;
	}

	payload.radiance_ = albedo * lighting;
	payload.hit_distance_ = RayTCurrent();
}
