#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/Raytracing/RaytracingDispatch.h>
#include <GraphicsEngine/Raytracing/VolumetricLight/VolumetricLightShader.h>

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
	* Tuning values of the fog and volumetric-light pass. Mirrors
	* VolumetricLightRayConstantBuffer in
	* Raytracing/VolumetricLight/VolumetricLight.hlsli, which the three froxel
	* passes and DeferredLightingPS.hlsl read through
	* constant_indices.volumetric_light_index_, so the layout must match the
	* HLSL side byte for byte.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フォグとボリューメトリックライトのパスの調整値。
	* Raytracing/VolumetricLight/VolumetricLight.hlsli の
	* VolumetricLightRayConstantBuffer と対応する。froxel の 3 パスと
	* DeferredLightingPS.hlsl が constant_indices.volumetric_light_index_ 経由で
	* 読むため、レイアウトは HLSL 側とバイト単位で一致させる。
	*/
	struct VolumetricLightRayConstantBuffer
	{
		/// [EN] Base fog density (scattering strength). The optical depth is the path length to the far plane times the extinction, so even 0.01 is thick fog that hides everything a few hundred units away; the default is a light haze.
		/// [JP] 基本のフォグ密度（散乱の強さ）。光学的深さは far plane までの経路長と消衰の積なので、0.01 でも数百単位先が見えない濃霧になる。既定値はうっすらとしたもや。
		Float density_ = 0.003f;

		/// [EN] Absorption coefficient; extinction is density plus absorption.
		/// [JP] 吸収係数。消衰は密度と吸収の和。
		Float absorption_ = 0.0005f;

		/// [EN] How fast the fog thins out with height; 0 gives uniform fog. The default keeps the fog near the ground.
		/// [JP] 高さに応じてフォグが薄くなる速さ。0 なら一様なフォグ。既定値は地表付近に霧が溜まる。
		Float heightFalloff_ = 0.05f;

		/// [EN] World height the height falloff is measured from.
		/// [JP] 高さによる減衰の基準となるワールドの高さ。
		Float heightReference_ = 0.0f;

		/// [EN] Fraction of light the fog scatters rather than absorbs, per color channel.
		/// [JP] フォグが吸収せずに散乱する光の割合。色チャンネルごと。
		Float fogAlbedo_[3] = { 1.0f, 1.0f, 1.0f };

		/// [EN] Henyey-Greenstein anisotropy; higher values give stronger god rays.
		/// [JP] Henyey-Greenstein の異方性。高いほどゴッドレイが強く出る。
		Float scatteringG_ = 0.7f;

		/// [EN] Maximum length of the shadow ray each froxel casts toward the sun.
		/// [JP] 各 froxel が太陽へ向けて飛ばすシャドウレイの最大長。
		Float rayTMax_ = 2000.0f;

		/// [EN] Multiplier on the sun in-scattering term, i.e. the strength of the god rays.
		/// [JP] 太陽の内散乱項の倍率。ゴッドレイの強さ。
		Float godrayStrength_ = 1.0f;

		/// [EN] Froxel grid dimensions. Written by VolumetricLightRenderer::Prepare, not by the editor UI.
		/// [JP] froxel グリッドの次元。エディターの UI ではなく VolumetricLightRenderer::Prepare が書き込む。
		Uint32 froxelDimensionX_ = 0;
		Uint32 froxelDimensionY_ = 0;
		Uint32 froxelDimensionZ_ = 0;

		/// [EN] 1 dims the sun by a short light march through the procedural clouds, giving crepuscular rays through cloud gaps.
		/// [JP] 1 ならプロシージャル雲の中を短くライトマーチして太陽を減光し、雲の切れ間から光芒を出す。
		Uint32 cloudShadowEnabled_ = 1;

		/// [EN] Pads the last row to a full 16-byte register.
		/// [JP] 最後の行を 16 バイトのレジスタ 1 つ分に揃える詰め物。
		Float volumetricLightPadding_[2] = { 0.0f, 0.0f };

		/**
		* [EN]
		* Reads or writes the tuning values through archive. The values
		* written by the renderer every frame and the padding are not
		* serialized.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* archive を通して調整値を読み書きする。レンダラーが毎フレーム書き込む
		* 値と詰め物はシリアライズしない。
		*/
		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.TryField("density", density_);
			archive.TryField("absorption", absorption_);
			archive.TryField("heightFalloff", heightFalloff_);
			archive.TryField("heightReference", heightReference_);
			archive.TryField("fogAlbedo", fogAlbedo_);
			archive.TryField("scatteringG", scatteringG_);
			archive.TryField("rayTMax", rayTMax_);
			archive.TryField("godrayStrength", godrayStrength_);
			archive.TryField("cloudShadowEnabled", cloudShadowEnabled_);
		}
	};

	/**
	* [EN]
	* Runs the froxel volumetric pipeline in three passes: FogInjectionCS
	* fills the medium, VolumetricLightScatteringRT lights it (sun occlusion
	* through inline RayQuery plus a cloud light march, which gives the god
	* rays), and FroxelIntegrationCS accumulates it front to back into the
	* integration volume that DeferredLightingPS.hlsl samples at each pixel's
	* depth. The scattering volume is jittered every frame and accumulated
	* over time, so each view owns its own history/write pair; the density
	* and integration volumes are shared and rewritten on every flush. The
	* froxel grid has a fixed size, so this pass has no Resize. When the pass
	* is off the integration volume is cleared to (0, 0, 0, 1) - no
	* scattering and full transmittance - so the composite changes nothing.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* froxel によるボリューメトリックのパイプラインを 3 パスで実行する。
	* FogInjectionCS が媒質を埋め、VolumetricLightScatteringRT がそれを照らし
	* （インライン RayQuery による太陽の遮蔽と、雲のライトマーチ。これが
	* ゴッドレイになる）、FroxelIntegrationCS が手前から奥へ積分して積分
	* ボリュームを作る。DeferredLightingPS.hlsl は各ピクセルの深度でそれを
	* サンプルする。散乱ボリュームは毎フレームずらして時間方向に蓄積するため、
	* ビューごとに履歴用と書き込み用の組を持つ。密度と積分のボリュームは共有で、
	* Flush のたびに書き直す。froxel グリッドは固定サイズなので、このパスには
	* Resize が無い。パスが無効なら積分ボリュームを (0, 0, 0, 1)（散乱なし、
	* 完全透過）でクリアするため、合成しても何も変わらない。
	*/
	class VolumetricLightRenderer
	{
	public:
		/**
		* [EN]
		* Binds the shared root signature and pipeline-state cache that the
		* three froxel shaders compile into.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* froxel の 3 つのシェーダーのコンパイル先となる、共有のルート
		* シグネチャとパイプラインステートキャッシュを関連付ける。
		*/
		VolumetricLightRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);

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
		~VolumetricLightRenderer() = default;

		/**
		* [EN]
		* Compiles the shaders, creates the tuning constant buffer, and
		* creates every froxel volume. width and height are accepted to match
		* the other ray-traced passes but are unused, since the grid has a
		* fixed size.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シェーダーをコンパイルし、調整値の定数バッファを作成して、すべての
		* froxel ボリュームを作成する。width と height は他のレイトレーシング
		* パスと形を揃えるために受け取るが、グリッドは固定サイズなので使わない。
		*/
		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Uploads the tuning values with the grid dimensions filled in,
		* records whether the pass runs this frame, and publishes this pass's
		* bindless indices to the index systems. It records no GPU work, and
		* must run before the index systems upload this frame's indices.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* グリッドの次元を埋めた調整値をアップロードし、今フレームにパスを実行
		* するかを記録して、このパスの bindless インデックスを各インデックス
		* システムへ公開する。GPU 処理は記録せず、各インデックスシステムが
		* 今フレームのインデックスをアップロードする前に呼び出す。
		*/
		void Prepare(const VolumetricLightRayConstantBuffer& settings, Bool enabled);

		/**
		* [EN]
		* Records the pass for view: the three froxel dispatches with
		* unordered-access barriers between them, or a clear of the
		* integration volume to (0, 0, 0, 1) when the pass is off, leaving the
		* integration volume in shader-resource state.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* view のパスを記録する。unordered-access バリアを挟んだ froxel の
		* 3 ディスパッチを行うか、パスが無効なら積分ボリュームを (0, 0, 0, 1) で
		* クリアし、積分ボリュームをシェーダーリソース状態で終える。
		*/
		void Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses, RaytracingView view);

	private:
		/// [EN] Size of the froxel grid: 160 x 90 screen tiles, 128 depth slices.
		/// [JP] froxel グリッドのサイズ。画面を 160 x 90 のタイルに分け、奥行きを 128 スライスに分ける。
		static constexpr Uint32 froxelDimensionX_ = 160;
		static constexpr Uint32 froxelDimensionY_ = 90;
		static constexpr Uint32 froxelDimensionZ_ = 128;

		/// [EN] Number of scattering volumes per view: one written this frame, one holding the previous frame as history.
		/// [JP] ビューごとの散乱ボリュームの数。今フレームに書き込む 1 つと、前フレームを履歴として持つ 1 つ。
		static constexpr Uint32 scatteringSlotCount_ = 2;

		/// [EN] Length of the Halton sequence used to jitter the froxel sample positions.
		/// [JP] froxel のサンプル位置をずらす Halton 列の長さ。
		static constexpr Uint32 froxelJitterSequenceLength_ = 16;

		/**
		* [EN]
		* Per-view, per-dispatch values the scattering pass reads through the
		* root constant: which slot to write, which to read as history, and
		* this frame's jitter.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 散乱パスがルート定数経由で読む、ビューごと・ディスパッチごとの値。
		* 書き込むスロット、履歴として読むスロット、今フレームのずらし量。
		*/
		struct VolumetricLightDispatchConstantBuffer
		{
			/// [EN] Bindless index of the scattering volume written this frame.
			/// [JP] 今フレームに書き込む散乱ボリュームの bindless インデックス。
			Uint scatteringWriteUnorderedAccessViewIndex_ = 0;

			/// [EN] Bindless index of the scattering volume read as history.
			/// [JP] 履歴として読む散乱ボリュームの bindless インデックス。
			Uint scatteringHistoryShaderResourceViewIndex_ = 0;

			/// [EN] 1 when the history holds a valid previous frame.
			/// [JP] 履歴が有効な前フレームを持っていれば 1。
			Uint historyValid_ = 0;

			/// [EN] This view's frame counter.
			/// [JP] このビューのフレームカウンター。
			Uint frameIndex_ = 0;

			/// [EN] Sub-froxel offset of this frame's samples, each axis in [-0.5, 0.5].
			/// [JP] 今フレームのサンプルの froxel 内でのずらし量。各軸 [-0.5, 0.5]。
			Vector3 jitter_ = { 0.0f, 0.0f, 0.0f };

			/// [EN] Pads jitter_ to a full 16-byte register.
			/// [JP] jitter_ を 16 バイトのレジスタ 1 つ分に揃える詰め物。
			Float volumetricLightDispatchPadding_ = 0.0f;
		};

		/**
		* [EN]
		* Temporal state of one view (editor or game): its pair of scattering
		* volumes and which of them is written next.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1 つのビュー（エディターかゲーム）の時間方向の状態。散乱ボリュームの
		* 組と、次にどちらへ書き込むか。
		*/
		struct View
		{
			/// [EN] The two scattering volumes: rgb is in-scattered light, a is extinction.
			/// [JP] 2 つの散乱ボリューム。rgb は内散乱の光、a は消衰。
			Microsoft::WRL::ComPtr<ID3D12Resource> scatteringVolumeResource_[scatteringSlotCount_];

			/// [EN] Resource state each scattering volume is currently in.
			/// [JP] 各散乱ボリュームの現在のリソース状態。
			D3D12_RESOURCE_STATES scatteringVolumeState_[scatteringSlotCount_] = { D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COMMON };

			/// [EN] Bindless indices of each scattering volume's write view.
			/// [JP] 各散乱ボリュームの書き込み用ビューの bindless インデックス。
			Uint32 scatteringVolumeUnorderedAccessViewIndex_[scatteringSlotCount_] = { 0, 0 };

			/// [EN] Bindless indices of each scattering volume's read view.
			/// [JP] 各散乱ボリュームの読み取り用ビューの bindless インデックス。
			Uint32 scatteringVolumeShaderResourceViewIndex_[scatteringSlotCount_] = { 0, 0 };

			/// [EN] Slot written by the next dispatch; the other slot is the history.
			/// [JP] 次のディスパッチで書き込むスロット。もう一方が履歴になる。
			Uint32 writeSlot_ = 0;

			/// [EN] Number of frames this view has accumulated, used to step the jitter sequence.
			/// [JP] このビューが蓄積したフレーム数。ずらし量の列を進めるのに使う。
			Uint32 frameIndex_ = 0;

			/// [EN] Whether the history slot holds a valid previous frame; false after a frame with the pass off.
			/// [JP] 履歴のスロットが有効な前フレームを持っているか。パスが無効だったフレームの後は false。
			Bool historyValid_ = false;

			/// [EN] GPU copy of this view's dispatch constants.
			/// [JP] このビューのディスパッチ用定数の GPU 側コピー。
			ResourcePtr<ConstantBuffer<VolumetricLightDispatchConstantBuffer>> constantBuffer_;
		};

		/// [EN] Injection, scattering and integration compute shaders of the pass.
		/// [JP] パスの注入、散乱、積分のコンピュートシェーダー。
		VolumetricLightShader volumetricLightShader_;

		/// [EN] GPU copy of the tuning values, read through its bindless index.
		/// [JP] 調整値の GPU 側コピー。bindless インデックス経由で読まれる。
		ResourcePtr<ConstantBuffer<VolumetricLightRayConstantBuffer>> tuningBuffer_;

		/// [EN] Output of pass 1: rgb is the scattering coefficient, a is extinction.
		/// [JP] パス 1 の出力。rgb は散乱係数、a は消衰。
		Microsoft::WRL::ComPtr<ID3D12Resource> densityVolumeResource_;

		/// [EN] Bindless index of the density volume's write view; the volume is only ever accessed through it.
		/// [JP] 密度ボリュームの書き込み用ビューの bindless インデックス。ボリュームはこのビューだけで読み書きする。
		Uint32 densityVolumeUnorderedAccessViewIndex_ = 0;

		/// [EN] Temporal state of the editor view.
		/// [JP] エディタービューの時間方向の状態。
		View editorView_;

		/// [EN] Temporal state of the game view.
		/// [JP] ゲームビューの時間方向の状態。
		View gameView_;

		/// [EN] Output of pass 3, sampled by the composite: rgb is accumulated scattering, a is transmittance.
		/// [JP] パス 3 の出力で、合成がサンプルする。rgb は累積の散乱、a は透過率。
		Microsoft::WRL::ComPtr<ID3D12Resource> integrationVolumeResource_;

		/// [EN] Resource state the integration volume is currently in.
		/// [JP] 積分ボリュームの現在のリソース状態。
		D3D12_RESOURCE_STATES integrationVolumeState_ = D3D12_RESOURCE_STATE_COMMON;

		/// [EN] Bindless index of the integration volume's write view.
		/// [JP] 積分ボリュームの書き込み用ビューの bindless インデックス。
		Uint32 integrationVolumeUnorderedAccessViewIndex_ = 0;

		/// [EN] Bindless index of the integration volume's read view.
		/// [JP] 積分ボリュームの読み取り用ビューの bindless インデックス。
		Uint32 integrationVolumeShaderResourceViewIndex_ = 0;

		/// [EN] Non-shader-visible heap holding the CPU-side write view that ClearUnorderedAccessViewFloat requires alongside the shader-visible one.
		/// [JP] ClearUnorderedAccessViewFloat がシェーダー可視のビューと併せて要求する、CPU 側の書き込み用ビューを置く非シェーダー可視ヒープ。
		DescriptorHeap clearHeap_;

		/// [EN] Index of the integration volume's write view inside clearHeap_.
		/// [JP] clearHeap_ 内での積分ボリュームの書き込み用ビューのインデックス。
		Uint32 clearIntegrationIndex_ = 0;

		/// [EN] Whether the density volume has made its one transition into unordered-access state, where it then stays for good.
		/// [JP] 密度ボリュームが 1 度だけの unordered-access 状態への遷移を済ませたか。以後はずっとその状態のまま。
		Bool workingVolumesTransitioned_ = false;

		/// [EN] Bindless heap that owns this pass's descriptors.
		/// [JP] このパスのディスクリプタを所有する bindless ヒープ。
		BindlessHeap* bindlessHeap_ = nullptr;

		/// [EN] Index systems that receive this pass's bindless indices in Prepare.
		/// [JP] Prepare でこのパスの bindless インデックスを受け取るインデックスシステム。
		ConstantIndicesSystem* constantIndicesSystem_ = nullptr;
		ShaderResourceIndicesSystem* shaderResourceIndicesSystem_ = nullptr;
		UnorderedAccessIndicesSystem* unorderedAccessIndicesSystem_ = nullptr;

		/// [EN] Whether the pass renders this frame.
		/// [JP] 今フレームにパスを描画するか。
		Bool enabled_ = false;

		/// [EN] Whether the missing-pipeline warning has been logged, so it appears once rather than every frame.
		/// [JP] パイプライン欠如の警告を出力済みか。毎フレームではなく 1 度だけ出すために使う。
		Bool pipelineStateMissingLogged_ = false;
	};
}
