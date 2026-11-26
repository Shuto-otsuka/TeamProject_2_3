#ifndef __VERTEX_HLSL__
#define __VERTEX_HLSL__

#include <GraphicsEngine/Shader/Normal.hlsli>

struct CompressedVertex
{
	uint position_xy_;
	uint position_z_texu_;
	uint texv_tangent_;
	uint normal_;
};

float3 DecodeCompressedVertexNormal(CompressedVertex vertex)
{
	return OctNormalDecode(float2(vertex.normal_ & 0xFFFF, vertex.normal_ >> 16) / 65535.0);
}

float2 DecodeCompressedVertexTexcoord(CompressedVertex vertex, float2 texcoord_min, float2 texcoord_extent)
{
	float2 texcoord01 = float2(vertex.position_z_texu_ >> 16, vertex.texv_tangent_ & 0xFFFF) / 65535.0;
	return texcoord_min + texcoord01 * texcoord_extent;
}

float4 DecodeCompressedVertexTangent(CompressedVertex vertex)
{
	uint tangent_bits = vertex.texv_tangent_ >> 16;
	float2 tangent_oct = float2(tangent_bits & 0xFF, (tangent_bits >> 8) & 0x7F) / float2(255.0, 127.0);
	float tangent_sign = (tangent_bits >> 15) ? 1.0 : -1.0;
	return float4(OctNormalDecode(tangent_oct), tangent_sign);
}

#endif
