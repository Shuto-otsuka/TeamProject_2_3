#ifndef __TRANSPARENT_HLSLI__
#define __TRANSPARENT_HLSLI__

#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Culling/Culling.hlsli>
#include <GraphicsEngine/Environment/Weather/Weather.hlsli>

struct TransparentDrawBindings
{
	uint geometry_instances_;
	uint culling_transparent_meshlets_;
	uint culling_transparent_arguments_;
	uint culling_transparent_chunks_;
	uint geometry_hi_z_;
	uint model_bones_;
	uint model_morphs_;
	uint light_list_;
	uint light_constants_;
	uint weather_constants_;
	uint light_clusters_;
	uint sky_diffuse_;
	uint sky_specular_;
	uint sky_brdf_;
	uint top_level_structure_;
	uint top_level_hits_;
	uint raytracing_shadow_constants_;
	uint transparent_heads_;
	uint transparent_fragments_;
	uint transparent_counter_;
	uint outline_styles_;
	uint outline_mask_;
};
ConstantBuffer<TransparentDrawBindings> bindings : register(b0, space1);

struct TransparentPayload
{
	uint2 entries_[32];
};

struct TransparentFragmentStructuredBuffer
{
	uint color_rg_;
	uint color_ba_;
	float depth_;
	uint next_;
	uint velocity_;
};

struct TransparentOutput
{
	float4 position : SV_Position;
	float3 world_position : POSITION0;
	float3 object_position : POSITION1;
	float3 normal : NORMAL0;
	float3 object_normal : NORMAL1;
	float4 tangent : TANGENT0;
	float3 object_tangent : TANGENT1;
	float2 texcoord : TEXCOORD0;
	float4 current_clip : TEXCOORD1;
	float4 previous_clip : TEXCOORD2;
	nointerpolation uint instance_index : BLENDINDICES0;
};

#endif
