#ifndef __VERTEX_HLSL__
#define __VERTEX_HLSL__

#include <GraphicsEngine/Shader/Normal.hlsli>

/**
* Mirrors the C++ CompressedVertex struct (Model/Crister/CristerGeometry.h) - 16 bytes. Used by
* the raytracing hit paths (RaytracingHit.hlsli: shadows, reflection, global
* illumination, refraction, ambient occlusion, subsurface scattering,
* volumetric light), which recover the normal, tangent and texcoord of a hit
* triangle. Model.hlsli's CompressedModelVertex is the same layout for the
* raster path.
*/
struct CompressedVertex
{
	uint position_xy_;
	uint position_z_texu_;
	uint texv_tangent_;
	uint normal_;
};

/**
* Decodes CompressedVertex's octahedral-packed normal_ field back into a
* world/object-space unit vector (Normal.hlsli: OctNormalDecode).
*/
float3 DecodeCompressedVertexNormal(CompressedVertex vertex)
{
	return OctNormalDecode(float2(vertex.normal_ & 0xFFFF, vertex.normal_ >> 16) / 65535.0);
}

/**
* Same unpacking as Model.hlsli's DecodeModelVertex - u is the high half of
* position_z_texu_, v the low half of texv_tangent_ - rescaled out of the
* mesh's UV AABB. Kept in sync with that: a mismatch here shows up as a
* raytraced effect sampling the wrong part of the texture.
*/
float2 DecodeCompressedVertexTexcoord(CompressedVertex vertex, float2 texcoord_min, float2 texcoord_extent)
{
	float2 texcoord01 = float2(vertex.position_z_texu_ >> 16, vertex.texv_tangent_ & 0xFFFF) / 65535.0;
	return texcoord_min + texcoord01 * texcoord_extent;
}

/**
* Decodes the octahedral tangent (x:8 y:7) in the high half of texv_tangent_
* and its handedness sign (bit 15, stored in w), as Model.hlsli's
* DecodeModelVertex does.
*/
float4 DecodeCompressedVertexTangent(CompressedVertex vertex)
{
	uint tangent_bits = vertex.texv_tangent_ >> 16;
	float2 tangent_oct = float2(tangent_bits & 0xFF, (tangent_bits >> 8) & 0x7F) / float2(255.0, 127.0);
	float tangent_sign = (tangent_bits >> 15) ? 1.0 : -1.0;
	return float4(OctNormalDecode(tangent_oct), tangent_sign);
}

#endif // __VERTEX_HLSL__
