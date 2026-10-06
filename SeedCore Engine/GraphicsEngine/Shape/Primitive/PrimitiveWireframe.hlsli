#ifndef __PRIMITIVE_WIREFRAME_HLSL__
#define __PRIMITIVE_WIREFRAME_HLSL__

#include "Primitive.hlsli"

/// [EN] The wireframe lines are built by the same code as the collider lines, so their instance layout, line counts and line geometry come from there.
#include "../Collider/ColliderLine.hlsli"

/**
* [EN]
* Constants of the primitive wireframe batch (where its instances are and
* how many), found through the view's primitive wireframe slot rather than
* the collider slot, so it is drawn independently of the colliders.
*/
ConstantBuffer<ColliderConstantBuffer> GetPrimitiveWireframeConstantBuffer()
{
    return ResourceDescriptorHeap[constant_indices.primitive_wireframe_index_];
}

#endif // __PRIMITIVE_WIREFRAME_HLSL__