#ifndef __GEOMETRY_HLSL__
#define __GEOMETRY_HLSL__

#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Culling/Culling.hlsli>

struct GeometryDrawBindings
{
	uint geometry_instances_;
	uint culling_opaque_meshlets_;
	uint culling_meshlet_arguments_;
	uint culling_opaque_chunks_;
	uint culling_occluded_chunks_;
	uint culling_chunk_arguments_;
	uint geometry_occluded_meshlets_;
	uint geometry_hi_z_history_;
	uint geometry_hi_z_;
	uint model_bones_;
	uint model_morphs_;
	uint geometry_debug_;
	uint geometry_visible_meshlets_;
};
ConstantBuffer<GeometryDrawBindings> bindings : register(b0, space1);

struct GeometryPayload
{
	uint2 entries_[32];
};

struct GeometryOutput
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
	float3 object_position : TEXCOORD1;
	float3 object_normal : NORMAL0;
	float4 object_tangent : TANGENT0;
	float3 world_position : TEXCOORD2;
	nointerpolation uint instance_index : BLENDINDICES0;
	nointerpolation uint meshlet_index : BLENDINDICES1;
};

struct GeometryPrimitive
{
	nointerpolation uint triangle_in_meshlet_index : BLENDINDICES2;
};

#endif
