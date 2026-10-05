#pragma once
#include <FoundationEngine/Prelude.h>
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
	* Pipelines for drawing primitive shapes as wireframe lines: one for the
	* editor view's debug overlay (light ranges, camera frustums, 3D physics
	* queries and so on) and one for the canvas view (2D physics queries).
	* The lines are built the same way as the collider lines, but the
	* instances are found through ConstantIndices::primitive_wireframe_index_,
	* so this batch is drawn independently of the colliders. Builds a mesh
	* shader pipeline on D12_2 and a vertex shader one below it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 基本形状をワイヤーフレームの線として描くパイプライン。エディター
	* ビューのデバッグの重ね描き用（ライトの範囲、カメラの視錐台、3D の物理
	* クエリなど）と、Canvas ビュー用（2D の物理クエリ）がある。線の
	* 作り方はコライダーの線と同じだが、インスタンスは
	* ConstantIndices::primitive_wireframe_index_ から見つけるので、コライダー
	* とは別に描ける。D12_2 ではメッシュシェーダ、それ未満では頂点シェーダの
	* パイプラインを作る。
	*/
	class PrimitiveWireframeShader
	{
	public:
		PrimitiveWireframeShader(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~PrimitiveWireframeShader() = default;

		/**
		* [EN]
		* Compiles the shaders and builds both pipeline states: the debug
		* overlay's, depth-tested without depth writes, and the canvas's,
		* without depth.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シェーダーをコンパイルし、2つのパイプラインステートを作る。デバッグの
		* 重ね描き用は深度テストを行い深度は書き込まない。Canvas 用は深度を
		* 使わない。
		*/
		void Create(ShaderCache& shaderCache, ID3D12Device* device);

		/**
		* [EN]
		* Returns the pipeline state for the editor view's debug overlay.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エディタービューのデバッグの重ね描き用のパイプラインステートを返す。
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
