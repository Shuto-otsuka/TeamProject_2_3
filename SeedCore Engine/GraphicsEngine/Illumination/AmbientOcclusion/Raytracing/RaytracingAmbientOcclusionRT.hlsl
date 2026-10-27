#include "../../../Shader/Scene.hlsli"
#include "../../../Shader/ShaderResources.hlsli"
#include "../../../Shader/UnorderedAccesses.hlsli"
#include "../../../Shader/Constants.hlsli"
#include "../../../Shader/Normal.hlsli"
#include "../../../Shader/Noise.hlsli"
#include "../../Reflection/Raytracing/RaytracingReflection.hlsli"
#include "RaytracingAmbientOcclusion.hlsli"

/**
* [EN]
* Distance to the closest occluder along ray_desc, or a negative value when the
* ray reaches ray_desc.TMax unoccluded. Same alpha handling as
* IsRayOccluded (RaytracingReflection.hlsli), but keeps the hit distance, which
* the openness falloff below needs.
*
* ---------------------------------------------------------------------
*
* [JP]
* ray_desc に沿った最も近い遮蔽物までの距離。TMax まで遮蔽が無ければ負値を
* 返す。アルファの扱いは IsRayOccluded(RaytracingReflection.hlsli)と同じだが、
* 下の開放度の減衰に必要なヒット距離を保持する。
*/
float AmbientOcclusionOccluderDistance(RaytracingAccelerationStructure tlas, RayDesc ray_desc, uint instance_data_index)
{
	RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;
	query.TraceRayInline(tlas, RAY_FLAG_NONE, 0xFF, ray_desc);

	while (query.Proceed())
	{
		if (query.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
		{
			if (!IsMaterialPassthrough(instance_data_index, query.CandidateInstanceID(), query.CandidatePrimitiveIndex(), query.CandidateTriangleBarycentrics()))
			{
				query.CommitNonOpaqueTriangleHit();
			}
		}
	}

	return query.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? query.CommittedRayT() : -1.0;
}

/**
* [EN]
* Reference:
* - http://www.realtimerendering.com/raytracinggems/rtg/index.html
*   (Ray Tracing Gems, Chapter 15 "On the Importance of Sampling" - cosine-
*   weighted hemisphere sampling for RTAO, ~30% lower error than uniform.)
* - https://docs.unity3d.com/Packages/com.unity.render-pipelines.high-definition@17.1/manual/Ray-Traced-Ambient-Occlusion.html
*   (Unity HDRP's RTAO - the distance-based falloff this pass mirrors,
*   instead of a hard binary cutoff at ray_length_.)
*
* Stochastic ray-traced ambient occlusion pass (inline RayQuery). One thread
* per screen pixel, ray_count_ cosine-weighted hemisphere rays per pixel per
* frame: reconstructs world position + normal from the G-Buffer, traces short
* rays (ray_length_) and writes the average openness, each ray's openness
* falling off with occluder distance (1 = open/no occluder within range, 0 =
* occluder essentially at the surface). The remaining noise is temporally
* accumulated by RaytracingAmbientOcclusionDenoiseCS.hlsl (same reprojection +
* neighborhood-clamp scheme as the shadow denoiser), which converges the
* running average toward the true hemisphere occlusion ratio.
*
* ---------------------------------------------------------------------
*
* [JP]
* 確率的レイトレAOパス(インライン RayQuery)。1スレッド=1ピクセル、
* 1フレームにつきコサイン重み付き半球レイを ray_count_ 本:
* G-Buffer からワールド座標と法線を復元し、短いレイ(ray_length_)を飛ばして
* 各レイの開放度を「遮蔽物までの距離で減衰する連続値」として求め、その
* 平均を書く(1=射程内に遮蔽物なし、0=遮蔽物がほぼ面上)。残るノイズは
* RaytracingAmbientOcclusionDenoiseCS.hlsl(影のデノイザと同じリプロジェクション+
* 近傍クランプ方式)が時間積分し、走り平均が真の半球遮蔽率へ収束する。
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
	RWTexture2D<float> raw_openness = ResourceDescriptorHeap[unordered_access_indices.ambient_occlusion_.raw_index_];

	Texture2D<float> depth_texture = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.depth_index_];
	float depth = depth_texture.Load(int3(pixel, 0));

	if (depth == 0.0)
	{
		raw_openness[pixel] = 1.0;
		return;
	}

	float2 uv = (float2(pixel) + 0.5) * scene.inverse_screen_size_;
	float2 ndc = float2(uv.x * 2 - 1, 1 - uv.y * 2);
	float4 clip = float4(ndc, depth, 1.0);
	float4 world = mul(clip, scene.inverse_view_projection_);
	float3 world_position = world.xyz / world.w;

	ConstantBuffer<RaytracingAmbientOcclusionConstantBuffer> tuning = ResourceDescriptorHeap[constant_indices.ambient_occlusion_index_];

	Texture2D<float4> normal_texture = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.index_1_];
	float3 normal = OctNormalDecode(normal_texture.Load(int3(pixel, 0)).rg);
	float3 origin = world_position + normal * tuning.normal_bias_;

	RaytracingAccelerationStructure tlas = ResourceDescriptorHeap[shader_resource_indices.raytracing_.tlas_index_];

	uint rng_state = SeedFromPixel(pixel, tuning.frame_index_ + 7919);

    float openness_sum = 0.0;
	
    for (uint index = 0; index < tuning.ray_count_; index++)
    {
		float3 ray_direction = CosineSampleHemisphere(rng_state, normal);

		RayDesc ray_desc;
		ray_desc.Origin = origin + ray_direction * tuning.normal_bias_;
		ray_desc.Direction = ray_direction;
		ray_desc.TMin = 0.001;
		ray_desc.TMax = tuning.ray_length_;

		float occluder_distance = AmbientOcclusionOccluderDistance(tlas, ray_desc, shader_resource_indices.raytracing_.instance_data_index_);
		
        openness_sum += occluder_distance < 0.0 ? 1.0 : saturate(occluder_distance / max(tuning.ray_length_, 0.0001));

    }
	
    raw_openness[pixel] = openness_sum / (float) tuning.ray_count_;
}
