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
	* Per-dispatch constants of SkinBlendCS: one dispatch skins the RT proxy of
	* one actor. Passed through the shared per-dispatch root constant as the
	* bindless index of a ConstantBuffer, and every buffer is a bindless index.
	* Mirrors SkinBlendDispatchBuffer in SkinBlend.hlsli.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* SkinBlendCS のディスパッチごとの定数。1 回のディスパッチで 1 体の
	* アクターの RT プロキシをスキニングする。ConstantBuffer の bindless
	* インデックスとして、共有のディスパッチごとのルート定数で渡し、
	* バッファは全て bindless インデックスで指す。SkinBlend.hlsli の
	* SkinBlendDispatchBuffer と対になる。
	*/
	struct SkinBlendDispatchBuffer
	{
		/// [EN] Number of vertices of the RT proxy.
		/// [JP] RT プロキシの頂点数。
		Uint32 vertexCount_ = 0;

		/// [EN] First bone matrix of the actor in the bone matrix buffer.
		/// [JP] ボーン行列バッファにおけるアクターの先頭ボーン行列。
		Uint32 boneOffset_ = 0;

		/// [EN] SRV of the float3 positions to skin: the base positions, or the morph-blended ones when the actor also has morphs.
		/// [JP] スキニングする float3 位置の SRV。ベース位置、またはアクターがモーフも持つならモーフブレンド済みの位置。
		Uint32 positionIndex_ = 0;

		/// [EN] SRV of the joints and weights of each RT proxy vertex.
		/// [JP] RT プロキシの各頂点のジョイントとウェイトの SRV。
		Uint32 skinVertexIndex_ = 0;

		/// [EN] SRV of the bone matrix buffer shared by every animated actor.
		/// [JP] アニメーションする全アクターで共有するボーン行列バッファの SRV。
		Uint32 boneMatrixIndex_ = 0;

		/// [EN] UAV of the float3 skinned positions, the vertex input of the actor's BLAS.
		/// [JP] スキン済み float3 位置の UAV。アクターの BLAS の頂点入力になる。
		Uint32 skinnedPositionIndex_ = 0;

		Uint32 skinBlendDispatchBufferPadding0_ = 0;
		Uint32 skinBlendDispatchBufferPadding1_ = 0;
	};

	/**
	* [EN]
	* Owns the pipeline of SkinBlendCS.hlsl. It uses the shared root signature
	* like every other pass, so it reads its inputs through the bindless heap
	* and SkinBlendDispatchBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* SkinBlendCS.hlsl のパイプラインを持つ。他の全パスと同じく共有の
	* ルートシグネチャを使うため、入力は bindless ヒープと
	* SkinBlendDispatchBuffer 経由で読む。
	*/
	class SkinBlendShader
	{
	public:
		SkinBlendShader(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~SkinBlendShader() = default;

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
