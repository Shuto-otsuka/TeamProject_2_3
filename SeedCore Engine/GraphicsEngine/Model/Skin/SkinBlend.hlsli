#ifndef __SKIN_BLEND_HLSL__
#define __SKIN_BLEND_HLSL__

#include "../../Shader/Dispatch.hlsli"

/**
* [EN]
* Per-dispatch constants of SkinBlendCS: one dispatch skins the RT proxy of
* one actor. Reached through the shared per-dispatch root constant
* (dispatch_buffer_index_, see Shader/Dispatch.hlsli), and every buffer is a
* bindless index into ResourceDescriptorHeap. Mirrors SkinBlendDispatchBuffer
* in SkinBlendShader.h.
*/
struct SkinBlendDispatchBuffer
{
	/// [EN] Number of vertices of the RT proxy.
	uint vertex_count_;

	/// [EN] First bone matrix of the actor in the bone matrix buffer.
	uint bone_offset_;

	/// [EN] SRV of the float3 positions to skin: the base positions, or the morph-blended ones when the actor also has morphs.
	uint position_index_;

	/// [EN] SRV of the CompressedModelSkin (joints and weights) of each RT proxy vertex.
	uint skin_vertex_index_;

	/// [EN] SRV of the ModelBoneMatrix buffer shared by every animated actor.
	uint bone_matrix_index_;

	/// [EN] UAV of the float3 skinned positions, the vertex input of the actor's BLAS.
	uint skinned_position_index_;

	uint skin_blend_dispatch_buffer_padding_0_;
	uint skin_blend_dispatch_buffer_padding_1_;
};

/**
* [EN]
* The SkinBlendDispatchBuffer of the current dispatch.
*/
ConstantBuffer<SkinBlendDispatchBuffer> GetSkinBlendDispatchBuffer()
{
	return ResourceDescriptorHeap[dispatch_buffer_index_];
}

#endif // __SKIN_BLEND_HLSL__
