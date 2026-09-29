#include "SkinBlend.hlsli"
#include "../Model.hlsli"

/**
* [EN]
* Skins the RT proxy positions of one actor with its bone matrices, one thread
* per vertex. The result is the vertex input of the actor's BLAS.
*
* ---------------------------------------------------------------------
*
* [JP]
* 1 体のアクターの RT プロキシの位置を、そのボーン行列でスキニングする。
* 1 頂点 1 スレッド。結果はアクターの BLAS の頂点入力になる。
*/
[NumThreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	SkinBlendDispatchBuffer dispatch_buffer = GetSkinBlendDispatchBuffer();

	uint index = id.x;
	if (index >= dispatch_buffer.vertex_count_)
	{
		return;
	}

	StructuredBuffer<float3> positions = ResourceDescriptorHeap[dispatch_buffer.position_index_];
	StructuredBuffer<CompressedModelSkin> skin_vertices = ResourceDescriptorHeap[dispatch_buffer.skin_vertex_index_];
	StructuredBuffer<ModelBoneMatrix> bone_matrices = ResourceDescriptorHeap[dispatch_buffer.bone_matrix_index_];
	RWStructuredBuffer<float3> skinned_positions = ResourceDescriptorHeap[dispatch_buffer.skinned_position_index_];

	ExpandedModelSkin skin = DecodeSkinVertex(skin_vertices[index]);
	float3 position = positions[index];

	/// [EN] A vertex bound to no joint stays where it is.
	/// [JP] どのジョイントにも結び付いていない頂点はその場に残す。
	if (dot(skin.weights_, 1.0) < 1e-5)
	{
		skinned_positions[index] = position;
		return;
	}

	/// [EN] Linear blend skinning: the weighted sum of the four joints' bone matrices.
	/// [JP] 線形ブレンドスキニング。4 つのジョイントのボーン行列の重み付き和。
	uint bone_offset = dispatch_buffer.bone_offset_;
	float4x4 skin_matrix =
		LoadBoneMatrix(bone_matrices[bone_offset + skin.joints_.x]) * skin.weights_.x +
		LoadBoneMatrix(bone_matrices[bone_offset + skin.joints_.y]) * skin.weights_.y +
		LoadBoneMatrix(bone_matrices[bone_offset + skin.joints_.z]) * skin.weights_.z +
		LoadBoneMatrix(bone_matrices[bone_offset + skin.joints_.w]) * skin.weights_.w;

	float3 skinned = mul(float4(position, 1.0), skin_matrix).xyz;

	/// [EN] The result becomes BLAS triangle vertices, and traversal over a non-finite vertex never terminates, so such a vertex keeps its unskinned position.
	/// [JP] 結果は BLAS の三角形頂点になり、非有限の頂点を含む走査は終わらないため、そうした頂点はスキン前の位置のままにする。
	if (!all(isfinite(skinned)))
	{
		skinned = position;
	}

	skinned_positions[index] = skinned;
}
