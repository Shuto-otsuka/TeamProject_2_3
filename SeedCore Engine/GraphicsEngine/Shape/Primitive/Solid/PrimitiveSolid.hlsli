#ifndef __PRIMITIVE_SOLID_HLSL__
#define __PRIMITIVE_SOLID_HLSL__

#include "../Primitive.hlsli"
#include "../../../Shader/Dispatch.hlsli"

/**
* [EN]
* What one amplification group hands to the mesh shader: the instance and
* those of its meshlets that survived culling.
*/
struct PrimitiveSolidASPayload
{
    uint instance_index;
    uint meshlet_indices[32];
};

struct PrimitiveSolidMSOutput
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint instance_index : INSTANCE0;
};

/**
* [EN]
* Where the culling compute shader writes its lists and indirect
* arguments. Same layout as ModelCullingConstantBuffer.
*/
struct PrimitiveSolidCullingConstantBuffer
{
    uint single_sided_index_;
    uint double_sided_index_;
    uint arguments_index_;
    uint primitive_solid_culling_constant_buffer_padding_0_;
};

ConstantBuffer<PrimitiveSolidCullingConstantBuffer> GetPrimitiveSolidCullingConstantBuffer()
{
    return ResourceDescriptorHeap[dispatch_buffer_index_];
}

/**
* [EN]
* Bindless indices of the solid batch's buffers, in the shader resource
* index table.
*/
struct PrimitiveSolidShaderResourceIndices
{
    uint instance_index_;
    uint vertex_index_;
    uint meshlet_index_;
    uint meshlet_bound_index_;

    uint vertex_indices_index_;
    uint primitive_indices_index_;
    uint2 primitive_solid_shader_resource_padding_0_;
};

/**
* [EN]
* One filled shape to draw: its pose, its size (what each component means
* depends on shape_kind_), and how its surfaces are colored.
*/
struct PrimitiveSolidStructuredBuffer
{
    float3 position_;
    float4 rotation_;
    float3 dimensions_;
    float4 color_;
    float2 uv_scale_;
    float2 uv_offset_;
    uint shape_kind_;
    uint texture_index_;
    uint double_sided_;
    uint meshlet_offset_;
    uint meshlet_count_;
};

StructuredBuffer<PrimitiveSolidStructuredBuffer> GetPrimitiveSolidStructuredBuffer(uint index)
{
    return ResourceDescriptorHeap[index];
}

/**
* [EN]
* One visible meshlet, written by the culling compute shader and read by
* the indirect vertex shader. Same layout as ModelCullingStructuredBuffer,
* since the buffers come from ModelCullingBuffer.
*/
struct PrimitiveSolidCullingStructuredBuffer
{
    uint instance_index_;
    uint meshlet_index_;
    uint2 primitive_solid_culling_structured_buffer_padding_0_;
};

StructuredBuffer<PrimitiveSolidCullingStructuredBuffer> GetPrimitiveSolidCullingStructuredBuffer(uint index)
{
    return ResourceDescriptorHeap[index];
}

#endif // __PRIMITIVE_SOLID_HLSL__