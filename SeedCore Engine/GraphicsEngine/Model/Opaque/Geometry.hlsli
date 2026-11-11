#ifndef __GEOMETRY_HLSL__
#define __GEOMETRY_HLSL__

#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Culling/Culling.hlsli>

struct GeometryDrawBindings
{
	uint model_instances_;
	uint culling_draw_lists_;
	uint culling_draw_arguments_;
	uint model_bones_;
	uint model_morph_weights_;
};
ConstantBuffer<GeometryDrawBindings> bindings : register(b0, space1);

/**
* [EN]
* Per-vertex output of GeometryMS/GeometryVS. The pixel shaders only need the
* texcoord (alpha cutout, VisibilityTexcoord) and the ids; everything else is
* rebuilt from the ids by the material resolve.
*/
struct GeometryOutput
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
	nointerpolation uint instance_index : BLENDINDICES0;
	nointerpolation uint meshlet_index : BLENDINDICES1;
};

/**
* [EN]
* Per-primitive output. The mesh shader pipeline does not generate
* SV_PrimitiveID, so the triangle's index inside its meshlet is passed
* explicitly (the vertex fallback writes it per vertex instead).
*/
struct GeometryPrimitive
{
	nointerpolation uint triangle_in_meshlet_index : BLENDINDICES2;
};

#endif // __GEOMETRY_HLSL__
