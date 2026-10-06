#ifndef __PRIMITIVE_HLSL__
#define __PRIMITIVE_HLSL__

#include "../../Shader/Scene.hlsli"

/**
* [EN]
* One vertex of a unit primitive mesh, shared by every instance of that
* shape. Mirrors the C++ PrimitiveVertex (36 bytes). cap_ is used by the
* capsule only: +1 on the top half, -1 on the bottom half.
*/
struct PrimitiveVertex
{
    float3 position_;
    float3 normal_;
    float2 texcoord_;
    float cap_;
};

/**
* [EN]
* One meshlet: where its vertices start in the vertex index table and how
* many there are, and where its triangles start in the primitive index
* table (in bytes, three per triangle) and how many there are. Mirrors the
* C++ MeshletDesc (16 bytes).
*/
struct MeshletDesc
{
    uint vertex_offset_;
    uint triangle_offset_;
    uint vertex_count_;
    uint triangle_count_;
};

#endif // __PRIMITIVE_HLSL__
