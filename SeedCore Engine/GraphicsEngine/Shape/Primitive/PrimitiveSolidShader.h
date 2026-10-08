#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/Handle.h>
#include <GraphicsEngine/D3D12/PipelineState/RootSignature.h>
#include <GraphicsEngine/D3D12/PipelineState/PipelineStateObject.h>

namespace SeedCore
{
	class ShaderCache;
	class AmplificationShader;
	class MeshShader;
	class ComputeShader;
	class VertexShader;
	class PixelShader;

	/**
	* [EN]
	* Pipelines for drawing primitive shapes as filled surfaces, in the same
	* two ways as the models. On D12_2 an amplification shader culls each
	* instance's meshlets and a mesh shader draws the survivors, dropping the
	* back faces of single-sided shapes per triangle. Below D12_2 a compute
	* shader culls the meshlets into a single-sided and a double-sided list,
	* and each list is drawn indirectly by a vertex shader with its own
	* rasterizer state. All draw into the post-tonemap 8-bit target with depth
	* testing and depth writes.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 基本形状を面で描くパイプライン。モデルと同じ2通りの描き方を持つ。
	* D12_2 では Amplification Shader が各インスタンスのメッシュレットを
	* カリングし、残ったものをメッシュシェーダーが描く。片面の形の裏面は
	* 三角形ごとに捨てる。D12_2 未満では Compute Shader がメッシュレットを
	* 片面用と両面用の一覧にカリングし、それぞれの一覧を、別々の
	* ラスタライザの設定の頂点シェーダーで間接描画する。どれもトーンマップ後の
	* 8 ビットの描画先に、深度テストと深度の書き込みをしながら描く。
	*/
	class PrimitiveSolidShader
	{
	public:
		PrimitiveSolidShader(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);
		~PrimitiveSolidShader() = default;

		/**
		* [EN]
		* Compiles the shaders for the current feature level and builds its
		* pipeline states: the amplification and mesh shader one on D12_2,
		* or the culling compute one and the single-sided and double-sided
		* vertex shader ones below it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 今の機能レベル用のシェーダーをコンパイルし、パイプラインステートを
		* 作る。D12_2 では Amplification Shader とメッシュシェーダーのもの、
		* それ未満ではカリングの Compute Shader のものと、片面用・両面用の
		* 頂点シェーダーのものを作る。
		*/
		void Create(ShaderCache& shaderCache, ID3D12Device* device);

		/**
		* [EN]
		* Returns the amplification and mesh shader pipeline state (D12_2).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Amplification Shader とメッシュシェーダーのパイプラインステートを
		* 返す（D12_2）。
		*/
		[[nodiscard]] ID3D12PipelineState* GetPipelineState()const;

		/**
		* [EN]
		* Returns the culling compute pipeline state (below D12_2).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* カリングの Compute Shader のパイプラインステートを返す（D12_2 未満）。
		*/
		[[nodiscard]] ID3D12PipelineState* GetPipelineStateCulling()const;

		/**
		* [EN]
		* Returns the vertex shader pipeline state that culls back faces, for
		* the single-sided list (below D12_2).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 裏面を捨てる頂点シェーダーのパイプラインステートを返す。片面用の
		* 一覧に使う（D12_2 未満）。
		*/
		[[nodiscard]] ID3D12PipelineState* GetPipelineStateSingleSided()const;

		/**
		* [EN]
		* Returns the vertex shader pipeline state that keeps both faces, for
		* the double-sided list (below D12_2).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 両面を残す頂点シェーダーのパイプラインステートを返す。両面用の
		* 一覧に使う（D12_2 未満）。
		*/
		[[nodiscard]] ID3D12PipelineState* GetPipelineStateDoubleSided()const;

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
		/// [EN] Amplification shader, used on D12_2.
		/// [JP] Amplification Shader。D12_2 で使う。
		Handle<AmplificationShader> amplificationShader_;

		/// [EN] Mesh shader, used on D12_2.
		/// [JP] メッシュシェーダー。D12_2 で使う。
		Handle<MeshShader> meshShader_;

		/// [EN] Culling compute shader, used below D12_2.
		/// [JP] カリングの Compute Shader。D12_2 未満で使う。
		Handle<ComputeShader> computeShader_;

		/// [EN] Indirect vertex shader, used below D12_2.
		/// [JP] 間接描画の頂点シェーダー。D12_2 未満で使う。
		Handle<VertexShader> vertexShader_;

		/// [EN] Pixel shader.
		/// [JP] ピクセルシェーダー。
		Handle<PixelShader> pixelShader_;

		/// [EN] Amplification and mesh shader pipeline state.
		/// [JP] Amplification Shader とメッシュシェーダーのパイプラインステート。
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineState_;

		/// [EN] Culling compute pipeline state.
		/// [JP] カリングの Compute Shader のパイプラインステート。
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineStateCulling_;

		/// [EN] Vertex shader pipeline state for the single-sided list.
		/// [JP] 片面用の一覧の頂点シェーダーのパイプラインステート。
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineStateSingleSided_;

		/// [EN] Vertex shader pipeline state for the double-sided list.
		/// [JP] 両面用の一覧の頂点シェーダーのパイプラインステート。
		Handle<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineStateDoubleSided_;

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
