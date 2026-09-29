#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/Handle.h>
#include <GraphicsEngine/D3D12/PipelineState/PipelineStateObject.h>
#include <GraphicsEngine/D3D12/PipelineState/RootSignature.h>

namespace SeedCore
{
	class ComputeShader;
	class ShaderCache;

	/**
	* [EN]
	* Per-dispatch constants of MorphBlendCS: one dispatch blends the morph
	* targets of one SubMesh of one actor. Passed through the shared
	* per-dispatch root constant as the bindless index of a ConstantBuffer, and
	* every buffer is a bindless index. Mirrors MorphBlendDispatchBuffer in
	* MorphBlend.hlsli.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* MorphBlendCS のディスパッチごとの定数。1 回のディスパッチで 1 体の
	* アクターの 1 つの SubMesh のモーフターゲットをブレンドする。
	* ConstantBuffer の bindless インデックスとして、共有のディスパッチごとの
	* ルート定数で渡し、バッファは全て bindless インデックスで指す。
	* MorphBlend.hlsli の MorphBlendDispatchBuffer と対になる。
	*/
	struct MorphBlendDispatchBuffer
	{
		/// [EN] First vertex of the SubMesh in the RT proxy.
		/// [JP] RT プロキシ内の SubMesh の先頭頂点。
		Uint32 vertexOffset_ = 0;

		/// [EN] Number of vertices of the SubMesh.
		/// [JP] SubMesh の頂点数。
		Uint32 vertexCount_ = 0;

		/// [EN] Number of morph targets of the SubMesh.
		/// [JP] SubMesh のモーフターゲット数。
		Uint32 targetCount_ = 0;

		/// [EN] First delta of the SubMesh in the morph delta pool, in float3 units.
		/// [JP] モーフデルタプール内の SubMesh の先頭デルタ。float3 単位。
		Uint32 morphDeltaOffset_ = 0;

		/// [EN] SRV of the base float3 positions of the RT proxy.
		/// [JP] RT プロキシのベース float3 位置の SRV。
		Uint32 positionIndex_ = 0;

		/// [EN] SRV of the float3 morph delta pool of the RT proxy.
		/// [JP] RT プロキシの float3 モーフデルタプールの SRV。
		Uint32 morphDeltaIndex_ = 0;

		/// [EN] SRV of this frame's weights, one float per morph target.
		/// [JP] 今フレームのウェイトの SRV。モーフターゲットごとに float 1 つ。
		Uint32 morphWeightIndex_ = 0;

		/// [EN] UAV of the float3 blended positions.
		/// [JP] ブレンド済み float3 位置の UAV。
		Uint32 blendedPositionIndex_ = 0;
	};

	/**
	* [EN]
	* Owns the pipeline of MorphBlendCS.hlsl. It uses the shared root signature
	* like every other pass, so it reads its inputs through the bindless heap
	* and MorphBlendDispatchBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* MorphBlendCS.hlsl のパイプラインを持つ。他の全パスと同じく共有の
	* ルートシグネチャを使うため、入力は bindless ヒープと
	* MorphBlendDispatchBuffer 経由で読む。
	*/
	class MorphBlendShader
	{
	public:
		MorphBlendShader(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~MorphBlendShader() = default;

		void Create(ShaderCache& shaderCache, ID3D12Device* device);

		[[nodiscard]] ID3D12RootSignature* GetRootSignature()const;

		[[nodiscard]] ID3D12PipelineState* GetPipelineState()const;

	private:
		Handle<ComputeShader> computeShader_;
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineStateObjectHandle_;
		Handle<RootSignature> rootSignatureHandle_;

		RootSignature& rootSignature_;
		PipelineStateObject& pipelineStateObject_;
	};
}
