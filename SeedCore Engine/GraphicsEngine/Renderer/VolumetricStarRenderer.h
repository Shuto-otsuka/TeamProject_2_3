#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/Raytracing/VolumetricStar/VolumetricStarShader.h>

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
	* One shooting-star slot, simulated on the CPU and uploaded inside the
	* tuning buffer. Mirrors ShootingStarInstance in
	* Raytracing/VolumetricStar/VolumetricStar.hlsli byte for byte.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 流れ星のスロット 1 つ分。CPU でシミュレーションし、調整値のバッファに
	* 入れてアップロードする。Raytracing/VolumetricStar/VolumetricStar.hlsli の
	* ShootingStarInstance とバイト単位で一致させる。
	*/
	struct ShootingStarInstance
	{
		/// [EN] Sky direction where the streak starts.
		/// [JP] 筋が始まる空の方向。
		Vector3 startDirection_ = { 0.0f, 1.0f, 0.0f };

		/// [EN] 0 means the slot is idle; above 0 it is the 0-1 position of the head along the streak.
		/// [JP] 0 ならスロットは空き。0 より大きければ、筋に沿った先端の位置（0 から 1）。
		Float progress_ = 0.0f;

		/// [EN] Sky direction where the streak ends.
		/// [JP] 筋が終わる空の方向。
		Vector3 endDirection_ = { 0.0f, 1.0f, 0.0f };

		/// [EN] Brightness multiplier of this particular star.
		/// [JP] この流れ星個別の明るさの倍率。
		Float brightness_ = 0.0f;
	};

	/// [EN] Number of shooting-star slots; at most this many streaks are in the sky at once.
	/// [JP] 流れ星のスロット数。同時に空に出る筋はこの数まで。
	SC_CONST Uint32 volumetricStarMaxShootingStars_ = 4;

	/**
	* [EN]
	* Tuning values of the star pass. Mirrors VolumetricStarRayConstantBuffer
	* in Raytracing/VolumetricStar/VolumetricStar.hlsli, which both
	* VolumetricStarRT.hlsl and DeferredLightingPS.hlsl read through
	* constant_indices.star_index_. The fields are laid out in rows of four
	* scalars (16 bytes), and the layout must match the HLSL side byte for
	* byte.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 星パスの調整値。Raytracing/VolumetricStar/VolumetricStar.hlsli の
	* VolumetricStarRayConstantBuffer と対応する。VolumetricStarRT.hlsl と
	* DeferredLightingPS.hlsl の両方が constant_indices.star_index_ 経由で読む。
	* フィールドは 4 スカラー（16 バイト）単位の行で並べ、レイアウトは HLSL 側と
	* バイト単位で一致させる。
	*/
	struct VolumetricStarRayConstantBuffer
	{
		/// [EN] Angular size (radians) of one cell of the star placement grid.
		/// [JP] 星の配置グリッドのセル 1 つの角度サイズ（ラジアン）。
		Float cellSize_ = 0.12f;

		/// [EN] 0-1 chance that a grid cell holds a star.
		/// [JP] グリッドのセルに星が出現する確率（0 から 1）。
		Float density_ = 0.35f;

		/// [EN] Overall brightness of the stars.
		/// [JP] 星全体の明るさ。
		Float brightness_ = 1.0f;

		/// [EN] How fast the stars twinkle.
		/// [JP] 星がまたたく速さ。
		Float twinkleSpeed_ = 2.0f;

		/// [EN] Base color of the stars.
		/// [JP] 星の基本色。
		Float color_[3] = { 0.9f, 0.95f, 1.0f };

		/// [EN] Smallest star size in grid-cell units. Kept small because real stars read as pinpoints; the glow is what makes them visible beyond that.
		/// [JP] 星の最小サイズ（グリッドのセル単位）。実際の星は点にしか見えないため小さく保ち、それ以上に見えるようにするのはグローの役目。
		Float sizeMin_ = 0.02f;

		/// [EN] Largest star size in grid-cell units.
		/// [JP] 星の最大サイズ（グリッドのセル単位）。
		Float sizeMax_ = 0.05f;

		/// [EN] Chance per second of a new shooting star at full night.
		/// [JP] 完全な夜のときに、1 秒あたりに新しい流れ星が出る確率。
		Float shootingStarChancePerSecond_ = 0.05f;

		/// [EN] Brightness of a shooting-star streak.
		/// [JP] 流れ星の筋の明るさ。
		Float shootingStarBrightness_ = 3.0f;

		/// [EN] Angular half-width (radians) of a shooting-star streak.
		/// [JP] 流れ星の筋の角度半幅（ラジアン）。
		Float shootingStarWidth_ = 0.0015f;

		/// [EN] How many of the slots the shader draws.
		/// [JP] シェーダーが描くスロットの数。
		Uint32 maxConcurrentShootingStars_ = volumetricStarMaxShootingStars_;

		/// [EN] 1 when the pass is on this frame. Written by VolumetricStarRenderer::Prepare, not by the editor UI.
		/// [JP] 今フレームにパスが有効なら 1。エディターの UI ではなく VolumetricStarRenderer::Prepare が書き込む。
		Uint32 enabled_ = 0;

		/// [EN] Strength of the soft halo outside the signed-distance star shape.
		/// [JP] 符号付き距離で表した星形の外側に広がる、柔らかいハローの強さ。
		Float glowIntensity_ = 0.8f;

		/// [EN] How fast the halo fades with signed distance; higher values give a tighter glow.
		/// [JP] 符号付き距離に対するハローの減衰速度。大きいほどハローが締まる。
		Float glowFalloff_ = 120.0f;

		/// [EN] Current shooting-star slots, filled in by VolumetricStarRenderer::Prepare every frame.
		/// [JP] 現在の流れ星スロット。毎フレーム VolumetricStarRenderer::Prepare が詰める。
		ShootingStarInstance activeShootingStars_[volumetricStarMaxShootingStars_];

		/**
		* [EN]
		* Reads or writes the tuning values through archive. The values
		* written by the renderer every frame are not serialized.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* archive を通して調整値を読み書きする。レンダラーが毎フレーム書き込む
		* 値はシリアライズしない。
		*/
		template<class Archive>
		void Serialize(Archive& archive)
		{
			archive.TryField("cellSize", cellSize_);
			archive.TryField("density", density_);
			archive.TryField("brightness", brightness_);
			archive.TryField("twinkleSpeed", twinkleSpeed_);
			archive.TryField("color", color_);
			archive.TryField("sizeMin", sizeMin_);
			archive.TryField("sizeMax", sizeMax_);
			archive.TryField("shootingStarChancePerSecond", shootingStarChancePerSecond_);
			archive.TryField("shootingStarBrightness", shootingStarBrightness_);
			archive.TryField("shootingStarWidth", shootingStarWidth_);
			archive.TryField("glowIntensity", glowIntensity_);
			archive.TryField("glowFalloff", glowFalloff_);
		}
	};

	/// [EN] The HLSL side is 12 rows of four scalars (4 tuning rows plus 4 shooting-star slots of 2 rows each); nothing checks the layout at runtime, so the size is asserted here.
	/// [JP] HLSL 側は 4 スカラーの行が 12 行（調整値 4 行と、2 行ずつの流れ星スロット 4 つ）。実行時にはレイアウトを検証できないため、ここでサイズを静的に検証する。
	static_assert(sizeof(VolumetricStarRayConstantBuffer) == 12 * 4 * sizeof(Float), "VolumetricStarRayConstantBuffer が VolumetricStar.hlsli とバイト単位で一致していません");

	/**
	* [EN]
	* Runs the star pass (VolumetricStarRT.hlsl, screen space, sky pixels
	* only) into an RGBA16F texture - rgb is premultiplied color, a is
	* coverage - and leaves it in shader-resource state for
	* DeferredLightingPS.hlsl to composite over the sky, the same way the
	* cloud texture is composited. It also simulates the shooting stars on
	* the CPU: their state has to persist from frame to frame, and with only
	* four slots the tuning buffer is the simplest way to carry it to the
	* shader.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 星パス（VolumetricStarRT.hlsl、スクリーン空間、空のピクセルのみ）を
	* RGBA16F テクスチャ（rgb は事前乗算済みの色、a はカバレッジ）へ
	* ディスパッチし、DeferredLightingPS.hlsl が雲と同じ方法で空の上に合成
	* できるよう、シェーダーリソース状態にしておく。流れ星の CPU 側の
	* シミュレーションも行う。その状態はフレームをまたいで保持する必要があり、
	* スロットが 4 つだけなので、調整値のバッファがシェーダーへ運ぶ最も単純な
	* 経路になる。
	*/
	class VolumetricStarRenderer
	{
	public:
		/**
		* [EN]
		* Binds the shared root signature and pipeline-state cache that the
		* star shader compiles into.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 星シェーダーのコンパイル先となる、共有のルートシグネチャと
		* パイプラインステートキャッシュを関連付ける。
		*/
		VolumetricStarRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);

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
		~VolumetricStarRenderer() = default;

		/**
		* [EN]
		* Compiles the shader, creates the tuning constant buffer, and
		* allocates the star texture at width x height.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シェーダーをコンパイルし、調整値の定数バッファを作成して、
		* width x height の星テクスチャを確保する。
		*/
		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Recreates the star texture for a new render size.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新しいレンダーサイズに合わせて星テクスチャを作り直す。
		*/
		void Resize(ID3D12Device* device, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Advances the shooting stars (progress, expiry and a spawn roll
		* scaled by nightFactor), uploads the tuning values with them and the
		* enabled flag, and publishes this pass's bindless indices to the
		* index systems. It records no GPU work, and must run before the index
		* systems upload this frame's indices.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 流れ星を進め（進行、満了、nightFactor に応じた出現の抽選）、それと
		* 有効フラグを入れた調整値をアップロードし、このパスの bindless
		* インデックスを各インデックスシステムへ公開する。GPU 処理は記録せず、
		* 各インデックスシステムが今フレームのインデックスをアップロードする前に
		* 呼び出す。
		*/
		void Prepare(const VolumetricStarRayConstantBuffer& settings, Bool enabled, Float deltaTime, Float nightFactor);

		/**
		* [EN]
		* Records the pass: renders the stars into the star texture, or clears
		* it to 0 when the pass is off, and leaves the texture in
		* shader-resource state. The G-Buffer depth must already be written,
		* since it tells sky pixels apart.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パスを記録する。星テクスチャへ星を描くか、パスが無効なら 0 で
		* クリアし、テクスチャをシェーダーリソース状態で終える。空のピクセルを
		* 見分けるため、G-Buffer の深度が書き込み済みであることが前提。
		*/
		void Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

	private:
		/**
		* [EN]
		* Creates the width_ x height_ star texture and its bindless and clear
		* descriptors.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* width_ x height_ の星テクスチャと、その bindless ディスクリプタおよび
		* クリア用ディスクリプタを作成する。
		*/
		void Allocate(ID3D12Device* device);

		/**
		* [EN]
		* Frees the star texture's bindless descriptors and defers the
		* texture's destruction until the GPU has finished with it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 星テクスチャの bindless ディスクリプタを解放し、テクスチャ自体の破棄は
		* GPU が使い終えるまで遅延させる。
		*/
		void Release();

	private:
		/// [EN] Compute shader and pipeline of VolumetricStarRT.hlsl.
		/// [JP] VolumetricStarRT.hlsl のコンピュートシェーダーとパイプライン。
		VolumetricStarShader starShader_;

		/// [EN] Random source for shooting-star spawning and placement.
		/// [JP] 流れ星の出現と配置に使う乱数源。
		std::mt19937 randomEngine_ = std::mt19937(std::random_device{}());

		/// [EN] GPU copy of the tuning values, read through its bindless index.
		/// [JP] 調整値の GPU 側コピー。bindless インデックス経由で読まれる。
		ResourcePtr<StaticConstantBuffer<VolumetricStarRayConstantBuffer>> tuningBuffer_;

		/// [EN] CPU-side state of each shooting-star slot, carried from frame to frame.
		/// [JP] 流れ星スロットごとの CPU 側の状態。フレームをまたいで引き継ぐ。
		ShootingStarInstance shootingStars_[volumetricStarMaxShootingStars_];

		/// [EN] Total flight time in seconds of the star in each slot.
		/// [JP] 各スロットの流れ星が飛ぶ総時間（秒）。
		Float shootingStarDuration_[volumetricStarMaxShootingStars_] = {};

		/// [EN] Star texture: rgb is premultiplied color, a is coverage.
		/// [JP] 星テクスチャ。rgb は事前乗算済みの色、a はカバレッジ。
		Microsoft::WRL::ComPtr<ID3D12Resource> starResource_;

		/// [EN] Resource state the star texture is currently in, tracked to issue only the barriers that are needed.
		/// [JP] 星テクスチャの現在のリソース状態。必要なバリアだけを発行するために追跡する。
		D3D12_RESOURCE_STATES starState_ = D3D12_RESOURCE_STATE_COMMON;

		/// [EN] Bindless index of the star texture's write view.
		/// [JP] 星テクスチャの書き込み用ビューの bindless インデックス。
		Uint32 starUnorderedAccessViewIndex_ = 0;

		/// [EN] Bindless index of the star texture's read view.
		/// [JP] 星テクスチャの読み取り用ビューの bindless インデックス。
		Uint32 starShaderResourceViewIndex_ = 0;

		/// [EN] Non-shader-visible heap holding the CPU-side write view that ClearUnorderedAccessViewFloat requires alongside the shader-visible one.
		/// [JP] ClearUnorderedAccessViewFloat がシェーダー可視のビューと併せて要求する、CPU 側の書き込み用ビューを置く非シェーダー可視ヒープ。
		DescriptorHeap clearHeap_;

		/// [EN] Index of the star texture's write view inside clearHeap_.
		/// [JP] clearHeap_ 内での星テクスチャの書き込み用ビューのインデックス。
		Uint32 clearIndex_ = 0;

		/// [EN] Bindless heap that owns this pass's descriptors.
		/// [JP] このパスのディスクリプタを所有する bindless ヒープ。
		BindlessHeap* bindlessHeap_ = nullptr;

		/// [EN] Index systems that receive this pass's bindless indices in Prepare.
		/// [JP] Prepare でこのパスの bindless インデックスを受け取るインデックスシステム。
		ConstantIndicesSystem* constantIndicesSystem_ = nullptr;
		ShaderResourceIndicesSystem* shaderResourceIndicesSystem_ = nullptr;
		UnorderedAccessIndicesSystem* unorderedAccessIndicesSystem_ = nullptr;

		/// [EN] Size of the star texture, equal to the native render size.
		/// [JP] 星テクスチャのサイズ。ネイティブのレンダーサイズと等しい。
		Uint32 width_ = 0;
		Uint32 height_ = 0;

		/// [EN] Whether the pass renders this frame.
		/// [JP] 今フレームにパスを描画するか。
		Bool enabled_ = false;

		/// [EN] Whether the missing-pipeline warning has been logged, so it appears once rather than every frame.
		/// [JP] パイプライン欠如の警告を出力済みか。毎フレームではなく 1 度だけ出すために使う。
		Bool pipelineStateMissingLogged_ = false;
	};
}
