#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Log/Assert.h>
#include <FoundationEngine/Utility/Handle.h>
#include <GraphicsEngine/D3D12/PipelineState/RootSignature.h>
#include <GraphicsEngine/D3D12/PipelineState/PipelineStateObject.h>
#include <GraphicsEngine/D3D12/PipelineState/DepthStencilState.h>

namespace SeedCore
{
	class ShaderCache;
	class VertexShader;
	class MeshShader;
	class PixelShader;

	/**
	* [EN]
	* One shape drawn as lines, for the GPU: its pose, its size (what each
	* component means depends on shapeKind_), its color and its line width
	* in pixels. headLength_ is used by the arrow only. Mirrors
	* PrimitiveWireframe.hlsli's PrimitiveWireframeStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 線で描く形1つ分の、GPU 向けのデータ。姿勢、大きさ（各成分の意味は
	* shapeKind_ で決まる）、色、ピクセル単位の線の太さを持つ。headLength_
	* は矢印のときだけ使う。PrimitiveWireframe.hlsli の
	* PrimitiveWireframeStructuredBuffer と一致する。
	*/
	struct PrimitiveWireframeStructuredBuffer
	{
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };
		Quaternion rotation_ = Quaternion::Identity;
		Vector3 dimensions_ = { 0.0f, 0.0f, 0.0f };
		Color color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		Uint32 shapeKind_ = 0;
		Float headLength_ = 0.0f;
		Float lineWidth_ = 0.0f;
	};
	SC_STATIC_ASSERT(PrimitiveWireframeStructuredBuffer, 68, "Shape/Primitive/Wireframe/PrimitiveWireframe.hlsli");

	/**
	* [EN]
	* Pipelines for drawing primitive shapes as wireframe lines: one for the
	* debug overlay of the editor and game views (light ranges, camera
	* frustums, colliders, physics queries and so on), one for the canvas
	* view (2D colliders and queries), and one for the preview views drawn
	* over everything (skeleton bones). The instances are found through
	* ShaderResourceIndices::primitive_wireframe_, which each view points at
	* its own batch. Builds mesh shader pipelines on D12_2 and vertex shader
	* ones below it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 基本形状をワイヤーフレームの線として描くパイプライン。エディターと
	* ゲームのビューのデバッグの重ね描き用（ライトの範囲、カメラの視錐台、
	* コライダー、物理クエリなど）、Canvas ビュー用（2D のコライダーと
	* クエリ）、すべての上に描くプレビュー用（スケルトンのボーン）がある。
	* インスタンスは ShaderResourceIndices::primitive_wireframe_ から見つけ、
	* 各ビューはそこに自分のバッチを指させる。D12_2 ではメッシュシェーダ、
	* それ未満では頂点シェーダのパイプラインを作る。
	*/
	class PrimitiveWireframeShader
	{
	public:
		/// [EN] Lines handled by one mesh shader group. Must match PrimitiveWireframeMS.hlsl.
		/// [JP] メッシュシェーダーの1グループが受け持つ線の数。PrimitiveWireframeMS.hlsl と一致させる。
		SC_CONST Uint linesPerGroup_ = 64;

		/// [EN] Groups one instance spans, enough for the capsule's 166 lines. Must match PrimitiveWireframeMS.hlsl.
		/// [JP] インスタンス1つがまたがるグループの数。カプセルの 166 本が入る数で、PrimitiveWireframeMS.hlsl と一致させる。
		SC_CONST Uint groupsPerInstance_ = 3;

		/// [EN] Vertices per line in the vertex shader path: a quad drawn as two triangles.
		/// [JP] 頂点シェーダーの経路での、線1本の頂点数。四角形を三角形2つで描く。
		SC_CONST Uint verticesPerLine_ = 6;

	public:
		PrimitiveWireframeShader(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~PrimitiveWireframeShader() = default;

		/**
		* [EN]
		* Compiles the shaders and builds the three pipeline states: the debug
		* overlay's, depth-tested without depth writes; the canvas's, without
		* depth; and the preview's, onto a 16-bit float target with a depth
		* buffer that is not tested, so the lines sit over the previewed model.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シェーダーをコンパイルし、3つのパイプラインステートを作る。デバッグの
		* 重ね描き用は深度テストを行い深度は書き込まない。Canvas 用は深度を
		* 使わない。プレビュー用は 16 ビット浮動小数の描画先へ、深度バッファは
		* あるがテストせずに描き、線がプレビューのモデルの上に乗るようにする。
		*/
		void Create(ShaderCache& shaderCache, ID3D12Device* device);

		/**
		* [EN]
		* Returns the pipeline state for the editor and game views' debug overlay.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エディターとゲームのビューのデバッグの重ね描き用のパイプライン
		* ステートを返す。
		*/
		[[nodiscard]] ID3D12PipelineState* GetPipelineState()const;

		/**
		* [EN]
		* Returns the pipeline state for the canvas view.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Canvas ビュー用のパイプラインステートを返す。
		*/
		[[nodiscard]] ID3D12PipelineState* GetPipelineStateCanvas()const;

		/**
		* [EN]
		* Returns the pipeline state for the preview views, drawn over
		* everything.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* すべての上に描く、プレビューのビュー用のパイプラインステートを返す。
		*/
		[[nodiscard]] ID3D12PipelineState* GetPipelineStatePreview()const;

		/**
		* [EN]
		* Returns the root signature.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ルートシグネチャを返す。
		*/
		[[nodiscard]] ID3D12RootSignature* GetRootSignature()const;

	private:
		/// [EN] Vertex shader, used below D12_2.
		/// [JP] 頂点シェーダ。D12_2 未満で使う。
		Handle<VertexShader> vertexShader_;

		/// [EN] Mesh shader, used on D12_2.
		/// [JP] メッシュシェーダ。D12_2 で使う。
		Handle<MeshShader> meshShader_;

		/// [EN] Pixel shader.
		/// [JP] ピクセルシェーダ。
		Handle<PixelShader> pixelShader_;

		/// [EN] Pipeline state for the debug overlay.
		/// [JP] デバッグの重ね描き用のパイプラインステート。
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineState_;

		/// [EN] Pipeline state for the canvas view.
		/// [JP] Canvas ビュー用のパイプラインステート。
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineStateCanvas_;

		/// [EN] Pipeline state for the preview views.
		/// [JP] プレビューのビュー用のパイプラインステート。
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineStatePreview_;

		/// [EN] Root signature handle.
		/// [JP] ルートシグネチャのハンドル。
		Handle<RootSignature> rootSignatureHandle_;

		/// [EN] Shared root signature store, owned by Renderer.
		/// [JP] 共有のルートシグネチャ。Renderer が所有する。
		RootSignature& rootSignature_;

		/// [EN] Shared pipeline state store, owned by Renderer.
		/// [JP] 共有のパイプラインステート。Renderer が所有する。
		PipelineStateObject& pipelineStateObject_;
	};
}
