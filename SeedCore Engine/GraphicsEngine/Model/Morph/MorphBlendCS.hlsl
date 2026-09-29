#include "MorphBlend.hlsli"

/**
* [EN]
* Blends the morph targets of one SubMesh into its vertex range of the RT
* proxy positions, one thread per vertex. Morph composes before skin: the
* result is either the vertex input of a morph-only actor's BLAS, or the input
* positions of SkinBlendCS. The caller pre-fills the blended positions with
* the base positions, so vertices outside this SubMesh keep them.
*
* ---------------------------------------------------------------------
*
* [JP]
* 1 つの SubMesh のモーフターゲットを、RT プロキシの位置のうちその頂点範囲へ
* ブレンドする。1 頂点 1 スレッド。モーフはスキンより前に合成する。結果は、
* モーフのみのアクターでは BLAS の頂点入力に、スキンもあるアクターでは
* SkinBlendCS の入力位置になる。呼び出し側がブレンド先をベース位置で
* 埋めておくので、この SubMesh の範囲外の頂点はベース位置のまま残る。
*/
[NumThreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	MorphBlendDispatchBuffer dispatch_buffer = GetMorphBlendDispatchBuffer();

	uint local_index = id.x;
	if (local_index >= dispatch_buffer.vertex_count_)
	{
		return;
	}

	StructuredBuffer<float3> positions = ResourceDescriptorHeap[dispatch_buffer.position_index_];
	StructuredBuffer<float3> morph_deltas = ResourceDescriptorHeap[dispatch_buffer.morph_delta_index_];
	StructuredBuffer<float> morph_weights = ResourceDescriptorHeap[dispatch_buffer.morph_weight_index_];
	RWStructuredBuffer<float3> blended_positions = ResourceDescriptorHeap[dispatch_buffer.blended_position_index_];

	uint global_index = dispatch_buffer.vertex_offset_ + local_index;
	float3 position = positions[global_index];

	/// [EN] The SubMesh's deltas are target-major: the delta of this vertex for a target sits at target * vertex_count_ + local_index.
	/// [JP] SubMesh のデルタはターゲット主順。あるターゲットでのこの頂点のデルタは target * vertex_count_ + local_index にある。
	for (uint target = 0; target < dispatch_buffer.target_count_; ++target)
	{
		position += morph_deltas[dispatch_buffer.morph_delta_offset_ + target * dispatch_buffer.vertex_count_ + local_index] * morph_weights[target];
	}

	/// [EN] The result becomes BLAS triangle vertices, and traversal over a non-finite vertex never terminates, so such a vertex keeps its base position.
	/// [JP] 結果は BLAS の三角形頂点になり、非有限の頂点を含む走査は終わらないため、そうした頂点はベース位置のままにする。
	if (!all(isfinite(position)))
	{
		position = positions[global_index];
	}

	blended_positions[global_index] = position;
}
