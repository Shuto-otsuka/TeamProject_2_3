#ifndef __MORPH_BLEND_HLSL__
#define __MORPH_BLEND_HLSL__

#include "../../Shader/Dispatch.hlsli"

/**
* [EN]
* Per-dispatch constants of MorphBlendCS: one dispatch blends the morph
* targets of one SubMesh of one actor. Reached through the shared per-dispatch
* root constant (dispatch_buffer_index_, see Shader/Dispatch.hlsli), and every
* buffer is a bindless index into ResourceDescriptorHeap. Mirrors
* MorphBlendDispatchBuffer in MorphBlendShader.h.
*/
struct MorphBlendDispatchBuffer
{
	/// [EN] First vertex of the SubMesh in the RT proxy.
	uint vertex_offset_;

	/// [EN] Number of vertices of the SubMesh.
	uint vertex_count_;

	/// [EN] Number of morph targets of the SubMesh.
	uint target_count_;

	/// [EN] First delta of the SubMesh in the morph delta pool, in float3 units. The SubMesh's deltas are target-major: target * vertex_count_ + vertex.
	uint morph_delta_offset_;

	/// [EN] SRV of the base float3 positions of the RT proxy.
	uint position_index_;

	/// [EN] SRV of the float3 morph delta pool of the RT proxy.
	uint morph_delta_index_;

	/// [EN] SRV of this frame's weights, one float per morph target.
	uint morph_weight_index_;

	/// [EN] UAV of the float3 blended positions, pre-filled with the base positions so vertices outside this SubMesh keep them.
	uint blended_position_index_;
};

/**
* [EN]
* The MorphBlendDispatchBuffer of the current dispatch.
*/
ConstantBuffer<MorphBlendDispatchBuffer> GetMorphBlendDispatchBuffer()
{
	return ResourceDescriptorHeap[dispatch_buffer_index_];
}

#endif // __MORPH_BLEND_HLSL__
