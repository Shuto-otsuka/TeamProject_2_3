#ifndef __GLOBAL_ILLUMINATION_HLSL__
#define __GLOBAL_ILLUMINATION_HLSL__

// The hit-surface geometry/material table is shared with the reflection pass:
// same TLAS, same instance order, same needs (vertex/index SRVs, base color,
// texture, UV AABB). GI reuses it rather than uploading a second copy - the
// type name says "Reflection" only because that pass introduced it.
#include "../../Reflection/Raytracing/RaytracingReflection.hlsli"

// RaytracingGlobalIlluminationReservoir/GlobalIlluminationReservoirCombine - the
// temporal ReSTIR resampling used by GlobalIlluminationRayGeneration. Needs
// Rand() (Noise.hlsli), which every includer of this file already pulls in
// before including it.
#include "RaytracingGlobalIlluminationReSTIR.hlsli"

/**
* GI tuning constant buffer, read by RaytracingGlobalIlluminationRT.hlsl and
* DeferredLightingPS.hlsl via
* constant_indices.global_illumination_index_. Must match the
* C++ mirror in Renderer/GlobalIlluminationRenderer.h byte-for-byte.
*/
struct RaytracingGlobalIlluminationConstantBuffer
{
	/// How far an indirect ray travels before it is treated as reaching the
	/// sky.
	float ray_t_max_;

	/// Offset along the normal to keep the ray off its own surface.
	float normal_bias_;

	/// Overall indirect intensity applied in DeferredLightingPS.hlsl.
	float intensity_;

	/// Per-frame counter, so the hemisphere sample direction changes every
	/// frame and the noise averages out over time instead of being a fixed
	/// pattern.
	uint frame_index_;

	uint temporal_reuse_enabled_;

	uint ray_count_;

	uint bounce_count_;

	float global_illumination_ray_padding_;
};

/**
* radiance(12) + hit_distance(4) + albedo(12) + normal(8) = 36 bytes, matching
* maxPayloadSizeInBytes in GlobalIlluminationShader.cpp. albedo_ and the
* oct-encoded normal_ describe the hit surface so raygen can continue the path
* from it for the next bounce.
*/
struct RaytracingGlobalIlluminationPayload
{
	float3 radiance_;
	float hit_distance_;
	float3 albedo_;
	float2 normal_;
};

#endif // __GLOBAL_ILLUMINATION_HLSL__
