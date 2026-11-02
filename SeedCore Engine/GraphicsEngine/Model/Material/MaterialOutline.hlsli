#ifndef __MATERIAL_OUTLINE_HLSL__
#define __MATERIAL_OUTLINE_HLSL__

#include "../../Shader/Dispatch.hlsli"

/**
* [EN]
* Bindless indices shared by the material outline passes
* (MaterialOutlineCS.hlsl and MaterialOutlinePS.hlsl). The resolve texture
* holds, per native-resolution pixel, the outline of the material that
* covers it (x = color packed as RGBA8, y = asuint(width)); the max-width
* buffer holds asuint() of the widest outline on screen this frame, which
* bounds the composite's search radius; the transparent record holds, per
* native-resolution pixel, (depth << 16) | (instance + 1) of the nearest
* outlined transparent fragment (0 when none), written by the transparent
* pass.
*/
struct MaterialOutlineConstantBuffer
{
	/// RWTexture2D<uint2> the resolve pass writes each pixel's material outline into.
	uint resolve_unordered_access_index_;

	/// Texture2D<uint2> view of the same resolve texture, read by the composite.
	uint resolve_shader_resource_index_;

	/// RWStructuredBuffer<uint> (1 element) holding asuint() of the widest outline on screen.
	uint max_width_unordered_access_index_;

	/// StructuredBuffer<uint> view of the same max-width buffer, read by the composite.
	uint max_width_shader_resource_index_;

	/// RWTexture2D<uint> transparent record, emptied by ClearTransparent and read by Resolve.
	uint transparent_unordered_access_index_;

	uint material_outline_padding_0_;
	uint material_outline_padding_1_;
	uint material_outline_padding_2_;
};

ConstantBuffer<MaterialOutlineConstantBuffer> GetMaterialOutlineConstantBuffer()
{
	return ResourceDescriptorHeap[dispatch_buffer_index_];
}

#endif // __MATERIAL_OUTLINE_HLSL__
