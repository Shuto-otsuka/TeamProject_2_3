#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/Raytracing/SubsurfaceScattering/SubsurfaceScatteringShader.h>

namespace SeedCore
{
	struct RootAddresses;

	class BindlessHeap;
	class ConstantIndicesSystem;
	class D3D12CommandList;
	class ShaderCache;
	class ShaderResourceIndicesSystem;
	class UnorderedAccessIndicesSystem;

	/**
	* [EN]
	* Tuning values of the subsurface scattering pass. Mirrors
	* SubsurfaceScatteringRayConstantBuffer in
	* Raytracing/SubsurfaceScattering/SubsurfaceScattering.hlsli, which both
	* SubsurfaceScatteringRT.hlsl and DeferredLightingPS.hlsl read through
	* constant_indices.subsurface_scattering_index_, so the layout must match
	* the HLSL side byte for byte.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 表面下散乱パスの調整値。
	* Raytracing/SubsurfaceScattering/SubsurfaceScattering.hlsli の
	* SubsurfaceScatteringRayConstantBuffer と対応する。
	* SubsurfaceScatteringRT.hlsl と DeferredLightingPS.hlsl の両方が
	* constant_indices.subsurface_scattering_index_ 経由で読むため、
	* レイアウトは HLSL 側とバイト単位で一致させる。
	*/
	struct SubsurfaceScatteringRayConstantBuffer
	{
		/// [EN] Characteristic falloff distance inside the object: transmittance = exp(-thickness / scatterDistance_). Smaller values make the object more opaque.
		/// [JP] オブジェクト内部での光の減衰特性距離（transmittance = exp(-厚み / scatterDistance_)）。小さいほど不透明になる。
		Float scatterDistance_ = 0.5f;

		/// [EN] Offset that pushes the thickness ray's origin into the surface, so the ray does not hit the face it starts on.
		/// [JP] 厚み計測レイの原点を表面の内側へ押し込むオフセット。撃ち出した面自身にヒットしないようにする。
		Float thicknessBias_ = 0.02f;

		/// [EN] Maximum thickness the ray searches; anything thicker counts as opaque.
		/// [JP] レイが探索する最大の厚み。これより厚い箇所は不透明として扱う。
		Float rayTMax_ = 5.0f;

		/// [EN] Overall translucency intensity applied in DeferredLightingPS.hlsl.
		/// [JP] DeferredLightingPS.hlsl で適用する透光の全体強度。
		Float strength_ = 1.0f;

		/// [EN] Tint of the transmitted light (reddish for skin, greenish for leaves, and so on).
		/// [JP] 透けた光の色（肌なら赤寄り、葉なら緑寄り、など）。
		Float subsurfaceColor_[3] = { 1.0f, 0.35f, 0.25f };

		/// [EN] Pads subsurfaceColor_ to a full 16-byte register.
		/// [JP] subsurfaceColor_ を 16 バイトのレジスタ 1 つ分に揃える詰め物。
		Float subsurfaceScatteringPadding_ = 0.0f;

		/**
		* [EN]
		* Reads or writes the tuning values through archive. The padding is
		* not serialized.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* archive を通して調整値を読み書きする。詰め物はシリアライズしない。
		*/
		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.TryField("scatterDistance", scatterDistance_);
			archive.TryField("thicknessBias", thicknessBias_);
			archive.TryField("rayTMax", rayTMax_);
			archive.TryField("strength", strength_);
			archive.TryField("subsurfaceColor", subsurfaceColor_);
		}
	};

	/**
	* [EN]
	* Dispatches the ray-traced subsurface scattering compute pass
	* (SubsurfaceScatteringRT.hlsl) into a single-channel transmittance
	* texture, and leaves it in shader-resource state for
	* DeferredLightingPS.hlsl to sample. The pass is deterministic - one
	* fixed thickness ray toward the directional light per pixel - so unlike
	* ShadowRenderer or AmbientOcclusionRenderer it has no denoiser, no
	* history accumulation and no per-view chain: one texture is written and
	* consumed within each view's flush, one view after the other. When the
	* pass is disabled, the scene has no TLAS this frame, or the compute
	* pipeline is unavailable, the texture is cleared to 0.0 (no
	* translucency) instead.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レイトレーシングによる表面下散乱のコンピュートパス
	* （SubsurfaceScatteringRT.hlsl）を 1 チャンネルの透過率テクスチャへ
	* ディスパッチし、DeferredLightingPS.hlsl がサンプルできるよう
	* シェーダーリソース状態にしておく。このパスは決定論的（1 ピクセルに
	* つきディレクショナルライトへ固定の厚み計測レイ 1 本）なので、
	* ShadowRenderer や AmbientOcclusionRenderer と違い、デノイザも履歴の
	* 蓄積もビューごとのチェーンも持たない。各ビューの Flush の中で書いて
	* 読むことを順番に行う 1 枚のテクスチャで足りる。パスが無効、今フレームの
	* TLAS が無い、またはコンピュートパイプラインが無い場合は、テクスチャを
	* 0.0（透光なし）でクリアする。
	*/
	class SubsurfaceScatteringRenderer
	{
	public:
		/**
		* [EN]
		* Binds the shared root signature and pipeline-state cache that the
		* subsurface scattering shader compiles into.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 表面下散乱シェーダーのコンパイル先となる、共有のルートシグネチャと
		* パイプラインステートキャッシュを関連付ける。
		*/
		SubsurfaceScatteringRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);

		/**
		* [EN]
		* Destroys the renderer. GPU resources are released together with the
		* bindless heap at shutdown.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レンダラーを破棄する。GPU リソースは終了時に bindless ヒープと共に
		* 解放される。
		*/
		~SubsurfaceScatteringRenderer() = default;

		/**
		* [EN]
		* Compiles the shader, creates the tuning constant buffer, and
		* allocates the transmittance texture at width x height. The index
		* systems are kept so Prepare can publish this pass's bindless
		* indices every frame.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シェーダーをコンパイルし、調整値の定数バッファを作成して、
		* width x height の透過率テクスチャを確保する。Prepare が毎フレーム
		* このパスの bindless インデックスを公開できるよう、各インデックス
		* システムを保持する。
		*/
		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Recreates the transmittance texture for a new render size.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新しいレンダーサイズに合わせて透過率テクスチャを作り直す。
		*/
		void Resize(ID3D12Device* device, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Uploads the tuning values, records whether the pass runs this frame,
		* and publishes this pass's bindless indices to the index systems. It
		* records no GPU work, and must run before the index systems upload
		* this frame's indices.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 調整値をアップロードし、今フレームにパスを実行するかを記録して、
		* このパスの bindless インデックスを各インデックスシステムへ公開する。
		* GPU 処理は記録せず、各インデックスシステムが今フレームのインデックスを
		* アップロードする前に呼び出す。
		*/
		void Prepare(const SubsurfaceScatteringRayConstantBuffer& settings, Bool enabled);

		/**
		* [EN]
		* Records the pass: traces SubsurfaceScatteringRT.hlsl into the
		* transmittance texture, or clears it to 0.0 when the pass is off, and
		* leaves the texture in shader-resource state. The G-Buffer depth and
		* normals must already be written.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パスを記録する。SubsurfaceScatteringRT.hlsl で透過率テクスチャへ
		* トレースするか、パスが無効なら 0.0 でクリアし、テクスチャを
		* シェーダーリソース状態で終える。G-Buffer の深度と法線が書き込み済み
		* であることが前提。
		*/
		void Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

	private:
		/**
		* [EN]
		* Creates the width_ x height_ transmittance texture and its bindless
		* and clear descriptors.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* width_ x height_ の透過率テクスチャと、その bindless ディスクリプタ
		* およびクリア用ディスクリプタを作成する。
		*/
		void Allocate(ID3D12Device* device);

		/**
		* [EN]
		* Frees the transmittance texture's bindless descriptors and defers
		* the texture's destruction until the GPU has finished with it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 透過率テクスチャの bindless ディスクリプタを解放し、テクスチャ自体の
		* 破棄は GPU が使い終えるまで遅延させる。
		*/
		void Release();

	private:
		/// [EN] Compute shader and pipeline of SubsurfaceScatteringRT.hlsl.
		/// [JP] SubsurfaceScatteringRT.hlsl のコンピュートシェーダーとパイプライン。
		SubsurfaceScatteringShader subsurfaceScatteringShader_;

		/// [EN] GPU copy of the tuning values, read through its bindless index.
		/// [JP] 調整値の GPU 側コピー。bindless インデックス経由で読まれる。
		ResourcePtr<ConstantBuffer<SubsurfaceScatteringRayConstantBuffer>> tuningBuffer_;

		/// [EN] Single-channel transmittance texture: 0 is fully opaque, 1 fully translucent.
		/// [JP] 1 チャンネルの透過率テクスチャ。0 が完全に不透明、1 が完全に透ける。
		Microsoft::WRL::ComPtr<ID3D12Resource> transmittanceResource_;

		/// [EN] Resource state the transmittance texture is currently in, tracked to issue only the barriers that are needed.
		/// [JP] 透過率テクスチャの現在のリソース状態。必要なバリアだけを発行するために追跡する。
		D3D12_RESOURCE_STATES transmittanceState_ = D3D12_RESOURCE_STATE_COMMON;

		/// [EN] Bindless index of the transmittance texture's write view.
		/// [JP] 透過率テクスチャの書き込み用ビューの bindless インデックス。
		Uint32 transmittanceUnorderedAccessViewIndex_ = 0;

		/// [EN] Bindless index of the transmittance texture's read view.
		/// [JP] 透過率テクスチャの読み取り用ビューの bindless インデックス。
		Uint32 transmittanceShaderResourceViewIndex_ = 0;

		/// [EN] Non-shader-visible heap holding the CPU-side write view that ClearUnorderedAccessViewFloat requires alongside the shader-visible one.
		/// [JP] ClearUnorderedAccessViewFloat がシェーダー可視のビューと併せて要求する、CPU 側の書き込み用ビューを置く非シェーダー可視ヒープ。
		DescriptorHeap clearHeap_;

		/// [EN] Index of the transmittance texture's write view inside clearHeap_.
		/// [JP] clearHeap_ 内での透過率テクスチャの書き込み用ビューのインデックス。
		Uint32 clearIndex_ = 0;

		/// [EN] Bindless heap that owns this pass's descriptors.
		/// [JP] このパスのディスクリプタを所有する bindless ヒープ。
		BindlessHeap* bindlessHeap_ = nullptr;

		/// [EN] Index systems that receive this pass's bindless indices in Prepare.
		/// [JP] Prepare でこのパスの bindless インデックスを受け取るインデックスシステム。
		ConstantIndicesSystem* constantIndicesSystem_ = nullptr;
		ShaderResourceIndicesSystem* shaderResourceIndicesSystem_ = nullptr;
		UnorderedAccessIndicesSystem* unorderedAccessIndicesSystem_ = nullptr;

		/// [EN] Size of the transmittance texture, equal to the native render size.
		/// [JP] 透過率テクスチャのサイズ。ネイティブのレンダーサイズと等しい。
		Uint32 width_ = 0;
		Uint32 height_ = 0;

		/// [EN] Whether the pass traces this frame: the pass is switched on and the scene has a TLAS.
		/// [JP] 今フレームにパスをトレースするか。パスが有効で、かつシーンに TLAS がある場合に true。
		Bool enabled_ = false;

		/// [EN] Whether the missing-pipeline warning has been logged, so it appears once rather than every frame.
		/// [JP] パイプライン欠如の警告を出力済みか。毎フレームではなく 1 度だけ出すために使う。
		Bool pipelineStateMissingLogged_ = false;
	};
}
