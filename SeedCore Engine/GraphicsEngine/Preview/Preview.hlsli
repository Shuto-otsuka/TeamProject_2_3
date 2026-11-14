#ifndef __PREVIEW_HLSL__
#define __PREVIEW_HLSL__

#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>

/**
* [EN]
* One line of Preview.Lines (bones, axes): world-space end points, the width
* in pixels and the color.
*/
struct PreviewLineStructuredBuffer
{
	float3 start_;
	float width_;
	float3 end_;
	float preview_line_structured_buffer_padding_0_;
	float4 color_;
};

struct PreviewMeshBindings
{
	uint preview_instances_;
	uint preview_meshlets_;
	uint preview_bones_;
};
ConstantBuffer<PreviewMeshBindings> mesh_bindings : register(b0, space1);

struct PreviewLineBindings
{
	uint preview_lines_;
};
ConstantBuffer<PreviewLineBindings> line_bindings : register(b0, space1);

/**
* [EN]
* Per-vertex output of PreviewMS/PreviewVS: the texcoord and the instance
* whose material the pixel shader reads.
*/
struct PreviewOutput
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
	nointerpolation uint instance_index : BLENDINDICES0;
};

/**
* [EN]
* Per-vertex output of PreviewLineMS/PreviewLineVS: the line's color.
*/
struct PreviewLineOutput
{
	float4 position : SV_Position;
	float4 color : COLOR0;
};

#endif // __PREVIEW_HLSL__
