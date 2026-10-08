#ifndef __PRIMITIVE_HLSL__
#define __PRIMITIVE_HLSL__

#include "../../Shader/Scene.hlsli"

/// [EN] Shape numbers, shared with the C++ ShapeKind.
#define PRIMITIVE_SHAPE_BOX 0
#define PRIMITIVE_SHAPE_SPHERE 1
#define PRIMITIVE_SHAPE_CAPSULE 2
#define PRIMITIVE_SHAPE_CYLINDER 3
#define PRIMITIVE_SHAPE_CONE 4
#define PRIMITIVE_SHAPE_RAMP 5
#define PRIMITIVE_SHAPE_TORUS 6

#define PRIMITIVE_SHAPE_RECT 7
#define PRIMITIVE_SHAPE_CIRCLE 8
#define PRIMITIVE_SHAPE_PLANE 9
#define PRIMITIVE_SHAPE_DISC 10
#define PRIMITIVE_SHAPE_SEGMENT 11

#define PRIMITIVE_SHAPE_ARROW 12

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
struct PrimitiveMeshlet
{
    uint vertex_offset_;
    uint triangle_offset_;
    uint vertex_count_;
    uint triangle_count_;
};

/**
* [EN]
* Culling bound of one meshlet in unit-mesh space: a bounding sphere and a
* normal cone. Mirrors the C++ MeshletBound and Model.hlsli's
* ModelMeshletBound (32 bytes). A cone_cutoff_ of 0 or below disables the
* cone test.
*/
struct PrimitiveMeshletBound
{
    float3 center_;
    float radius_;
    float3 cone_axis_;
    float cone_cutoff_;
};

#endif // __PRIMITIVE_HLSL__
