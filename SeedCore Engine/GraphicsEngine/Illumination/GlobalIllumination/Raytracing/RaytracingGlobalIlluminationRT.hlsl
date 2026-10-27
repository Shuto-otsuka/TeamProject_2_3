#include "RaytracingGlobalIllumination.hlsli"
#include "../../../Shader/Scene.hlsli"
#include "../../../Shader/Constants.hlsli"
#include "../../../Light/Light.hlsli"
#include "../../../Shader/Normal.hlsli"
#include "../../../Shader/Noise.hlsli"
#include "../../../Shader/Sampler.hlsli"
#include "../../../Shader/Vertex.hlsli"
#include "../../../Sky/IBL/ImageBasedLighting.hlsli"
#include "../../../Environment/VolumetricCloud/VolumetricCloudScapes.hlsli"
#include "../../../Sky/SkyGenerate.hlsli"
#include "../../../Shader/ShaderResources.hlsli"
#include "../../../Shader/UnorderedAccesses.hlsli"

/**
* [EN]
* Reference:
* - https://research.nvidia.com/publication/2021-06_restir-gi-path-resampling-real-time-path-tracing
*   (Ouyang et al., "ReSTIR GI: Path Resampling for Real-Time Path Tracing",
*   HPG 2021 - the ReSTIR reservoir reuse this raygen and the following
*   spatial pass are built on; see RaytracingGlobalIlluminationReSTIR.hlsli for why the
*   canonical receiver-side target function/Jacobian from that paper are NOT
*   used here.)
* - https://intro-to-restir.cwyman.org/presentations/2023ReSTIR_Course_Notes.pdf
*   (Wyman et al., ReSTIR course notes - background on RIS/target-function
*   weighting shared by every ReSTIR variant in this engine.)
*
* Ray-traced diffuse global illumination (RTPSO / DispatchRays: raygen + miss +
* closesthit). Fires ray_count_ cosine-weighted hemisphere rays per pixel per
* frame from the G-Buffer surface, each followed for up to bounce_count_
* bounces: miss returns sky radiance, closesthit relights the hit surface (sun +
* shadow ray + base color texture) so each bounce carries colour bleeding
* rather than just sky visibility, and raygen continues the path from the hit
* surface with a new cosine-weighted ray. Diffuse IBL is added only at the last
* vertex of a path, standing in for every bounce beyond it. Each ray's path
* becomes one candidate in the ReSTIR reservoir. raygen writes the incoming
* radiance; the ALBEDO of the receiving surface is applied in
* DeferredLightingPS.hlsl.
*
* ---------------------------------------------------------------------
*
* [JP]
* レイトレ拡散グローバルイルミネーション(RTPSO / DispatchRays:
* raygen + miss + closesthit)。G-Buffer の面から、1ピクセル1フレームにつき
* コサイン重み付き半球レイを ray_count_ 本撃ち、それぞれ最大 bounce_count_ 回
* バウンスさせる。miss は空の放射輝度を返し、closesthit はヒット面を
* 再ライティング(太陽+影レイ+ベースカラーテクスチャ)するので、各バウンスが
* 単なる空可視性ではなくカラーブリーディングを運ぶ。raygen はヒット面から
* 新しいコサイン重み付きレイでパスを続ける。拡散IBLはパスの最後の頂点でだけ
* 足し、それ以降の全バウンスの代わりにする。各レイのパスは ReSTIR reservoir の
* 1候補になる。raygen は入射放射輝度を書き、受け側の面のアルベドは
* DeferredLightingPS.hlsl で掛ける。
*/

[shader("raygeneration")]
void GlobalIlluminationRayGeneration()
{
	uint2 pixel = DispatchRaysIndex().xy;
	RWTexture2D<float4> output = ResourceDescriptorHeap[unordered_access_indices.global_illumination_.output_index_];

	Texture2D<float> depth_texture = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.depth_index_];
	float depth = depth_texture.Load(int3(pixel, 0));

	if (depth == 0.0)
	{
		output[pixel] = float4(0, 0, 0, 0);
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<RaytracingGlobalIlluminationConstantBuffer> tuning = ResourceDescriptorHeap[constant_indices.global_illumination_index_];

	float2 uv = (float2(pixel) + 0.5) * scene.inverse_screen_size_;
	float2 ndc = float2(uv.x * 2 - 1, 1 - uv.y * 2);
	float4 clip = float4(ndc, depth, 1.0);
	float4 world = mul(clip, scene.inverse_view_projection_);
	float3 world_position = world.xyz / world.w;

	Texture2D<float4> normal_texture = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.index_1_];
	float3 normal = OctNormalDecode(normal_texture.Load(int3(pixel, 0)).rg);

	uint rng_state = SeedFromPixel(pixel, tuning.frame_index_ + 15486071u);

	RaytracingAccelerationStructure tlas = ResourceDescriptorHeap[shader_resource_indices.raytracing_.tlas_index_];

	RaytracingGlobalIlluminationReservoir reservoir = (RaytracingGlobalIlluminationReservoir)0;

	for (uint ray_index = 0; ray_index < tuning.ray_count_; ray_index++)
	{
		float3 ray_direction = CosineSampleHemisphere(rng_state, normal);

		float3 bounce_origin = world_position + normal * tuning.normal_bias_;
		float3 bounce_direction = ray_direction;
		float3 throughput = float3(1, 1, 1);
		float3 path_radiance = float3(0, 0, 0);
		float first_hit_distance = 0.0;

		for (uint bounce = 0; bounce < tuning.bounce_count_; bounce++)
		{
			RayDesc ray_desc;
			ray_desc.Origin = bounce_origin;
			ray_desc.Direction = bounce_direction;
			ray_desc.TMin = 0.001;
			ray_desc.TMax = tuning.ray_t_max_;

			RaytracingGlobalIlluminationPayload payload;
			payload.radiance_ = float3(0, 0, 0);
			payload.hit_distance_ = 0.0;
			payload.albedo_ = float3(0, 0, 0);
			payload.normal_ = float2(0, 0);

			TraceRay(tlas, RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES, 0xFF, 0, 0, 0, ray_desc, payload);

			path_radiance += throughput * payload.radiance_;

			if (bounce == 0)
			{
				first_hit_distance = payload.hit_distance_;
			}

			if (payload.hit_distance_ >= 100000.0)
			{
				break;
			}

			float3 hit_normal = OctNormalDecode(payload.normal_);
			throughput *= payload.albedo_;

			if (bounce + 1 >= tuning.bounce_count_)
			{
				if (shader_resource_indices.sky_.diffuse_irradiance_index_ != 0)
				{
					TextureCube<float4> diffuse_irradiance = ResourceDescriptorHeap[shader_resource_indices.sky_.diffuse_irradiance_index_];
					path_radiance += throughput * diffuse_irradiance.SampleLevel(sampler_linear_clamp, hit_normal, 0).rgb * GetSkyConstantBuffer().intensity_;
				}

				break;
			}

			bounce_origin = bounce_origin + bounce_direction * payload.hit_distance_ + hit_normal * tuning.normal_bias_;
			bounce_direction = CosineSampleHemisphere(rng_state, hit_normal);
		}

		float3 candidate_position = world_position + ray_direction * first_hit_distance;
		RaytracingGlobalIlluminationReservoir candidate = GlobalIlluminationReservoirFromSample(candidate_position, normal, path_radiance);

		if (ray_index == 0)
		{
			reservoir = candidate;
		}
		else
		{
			reservoir = GlobalIlluminationReservoirCombine(reservoir, candidate, true, rng_state);
		}
	}

	reservoir.receiver_normal_ = normal;
	reservoir.receiver_depth_ = abs(mul(float4(world_position, 1.0), scene.non_jitter_view_projection_).w);

	StructuredBuffer<RaytracingGlobalIlluminationReservoir> reservoir_history = ResourceDescriptorHeap[shader_resource_indices.global_illumination_accumulation_.reservoir_history_index_];

	Texture2D<float2> velocity_texture = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.index_2_];
	float2 velocity = velocity_texture.Load(int3(pixel, 0)).rg;
	float2 previous_uv = uv - float2(velocity.x, -velocity.y);

	bool temporal_valid = tuning.temporal_reuse_enabled_ != 0 && all(previous_uv >= 0.0) && all(previous_uv <= 1.0);

	if (temporal_valid)
	{
		uint2 previous_pixel = uint2(previous_uv * scene.screen_size_);
		uint previous_index = previous_pixel.y * (uint)scene.screen_size_.x + previous_pixel.x;

		RaytracingGlobalIlluminationReservoir temporal = reservoir_history[previous_index];
		temporal.sample_m_ = min(temporal.sample_m_, GI_RESERVOIR_M_CAP);
		temporal.sample_age_ += 1.0;

		float expected_previous_depth = abs(mul(float4(world_position, 1.0), scene.previous_non_jitter_view_projection_).w);
		bool history_valid = temporal.receiver_depth_ > 0.0 && abs(temporal.receiver_depth_ - expected_previous_depth) <= GI_RESERVOIR_DEPTH_THRESHOLD * expected_previous_depth && dot(normal, temporal.receiver_normal_) >= GI_RESERVOIR_NORMAL_THRESHOLD && temporal.sample_age_ <= GI_RESERVOIR_MAX_AGE;

		reservoir = GlobalIlluminationReservoirCombine(reservoir, temporal, history_valid, rng_state);
	}

	RWStructuredBuffer<RaytracingGlobalIlluminationReservoir> reservoir_write = ResourceDescriptorHeap[unordered_access_indices.global_illumination_accumulation_.reservoir_index_];
	reservoir_write[pixel.y * (uint)scene.screen_size_.x + pixel.x] = reservoir;

	output[pixel] = float4(reservoir.sample_radiance_ * reservoir.sample_w_ * tuning.intensity_, 1.0);
}

[shader("miss")]
void GlobalIlluminationMiss(inout RaytracingGlobalIlluminationPayload payload)
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
void GlobalIlluminationAnyHit(inout RaytracingGlobalIlluminationPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
	if (IsMaterialPassthrough(shader_resource_indices.raytracing_.instance_data_index_, InstanceID(), PrimitiveIndex(), attributes.barycentrics))
	{
		IgnoreHit();
	}
}

[shader("closesthit")]
void GlobalIlluminationClosestHit(inout RaytracingGlobalIlluminationPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
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

	if (dot(world_normal, WorldRayDirection()) > 0.0)
	{
		world_normal = -world_normal;
	}

	float3 hit_position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<LightConstantBuffer> light = ResourceDescriptorHeap[constant_indices.light_index_];

	float3 lighting = float3(0, 0, 0);

	if (GetDirectionalLightConstantBuffer().sun_intensity_ > 0.0)
	{
		float3 light_direction = normalize(-GetDirectionalLightConstantBuffer().direction_);
		float normal_dot_light = saturate(dot(world_normal, light_direction));

		float sun_visibility = 1.0;

		if (normal_dot_light > 0.0)
		{
			ConstantBuffer<RaytracingGlobalIlluminationConstantBuffer> tuning = ResourceDescriptorHeap[constant_indices.global_illumination_index_];
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
	payload.albedo_ = albedo;
	payload.normal_ = OctNormalEncode(world_normal);
}
