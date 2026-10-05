#pragma once
#include <FoundationEngine/Prelude.h>
#include <GraphicsEngine/D3D12/Buffer/ConstantBuffer.h>
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/Raytracing/VolumetricCloudScapes/VolumetricCloudScapesShader.h>

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
	* Tuning values of the cloud and procedural-sky pass. Mirrors
	* VolumetricCloudScapesRayConstantBuffer in
	* Raytracing/VolumetricCloudScapes/VolumetricCloudScapes.hlsli, which both
	* VolumetricCloudScapesRT.hlsl and DeferredLightingPS.hlsl read through
	* constant_indices.cloud_index_. The fields are laid out in rows of four
	* scalars (one 16-byte cbuffer row each); new fields must keep that
	* grouping, and the layout must match the HLSL side byte for byte.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 雲とプロシージャル空のパスの調整値。
	* Raytracing/VolumetricCloudScapes/VolumetricCloudScapes.hlsli の
	* VolumetricCloudScapesRayConstantBuffer と対応する。
	* VolumetricCloudScapesRT.hlsl と DeferredLightingPS.hlsl の両方が
	* constant_indices.cloud_index_ 経由で読む。フィールドは 4 スカラー
	* （cbuffer の 16 バイトの行 1 つ）単位で並べる。フィールドを足すときも
	* この区切りを保ち、レイアウトは HLSL 側とバイト単位で一致させる。
	*/
	struct VolumetricCloudScapesRayConstantBuffer
	{
		/// [EN] Altitude of the cloud layer's bottom, measured above the curved ground (see CloudAltitudeAt) rather than as a flat world height.
		/// [JP] 雲層の下端の高度。平らなワールドの高さではなく、曲率のある地面からの高度（CloudAltitudeAt 参照）。
		Float cloudBottom_ = 500.0f;

		/// [EN] Altitude of the cloud layer's top, measured the same way.
		/// [JP] 雲層の上端の高度。同じ基準で測る。
		Float cloudTop_ = 1500.0f;

		/// [EN] Cloud amount, 0 to 1.
		/// [JP] 雲量（0 から 1）。
		Float coverage_ = 0.5f;

		/// [EN] Extinction scale of the density field.
		/// [JP] 密度場の消散スケール。
		Float densityScale_ = 0.02f;

		/// [EN] Fraction of light the cloud scatters rather than absorbs, per color channel.
		/// [JP] 雲が吸収せずに散乱する光の割合。色チャンネルごと。
		Float cloudAlbedo_[3] = { 1.0f, 1.0f, 1.0f };

		/// [EN] Henyey-Greenstein anisotropy of the forward lobe: 0 is isotropic, about 0.6 strongly forward. The backward lobe is set by phaseBackward_ and phaseBlend_.
		/// [JP] 前方ローブの Henyey-Greenstein 異方性。0 は等方、0.6 程度で強い前方散乱。後方ローブは phaseBackward_ と phaseBlend_ で決める。
		Float scatteringG_ = 0.6f;

		/// [EN] View-ray sample budget the marcher spends adaptively. The fine step is layerThickness / stepCount_, so 64 over a 1000-unit layer is a 15.6-unit step, finer than the smallest shape-noise feature (about 39 units).
		/// [JP] マーチャーが適応的に使う視線レイのサンプル数。細かいステップ長は 層の厚み / stepCount_ なので、1000 単位の層に 64 なら 15.6 単位となり、形状ノイズの最小の特徴（約 39 単位）より細かい。
		Uint32 stepCount_ = 64;

		/// [EN] Sun light-march steps taken from each view sample inside the cloud; this dominates the pass's cost. The steps grow geometrically, so a few of them still span the layer.
		/// [JP] 雲の中の視線サンプルごとに太陽方向へ行うライトマーチのステップ数。このパスのコストを支配する。ステップは指数的に伸びるため、少ない数でも層を張れる。
		Uint32 lightStepCount_ = 6;

		/// [EN] Scale from world position to the shape-noise domain; smaller values make larger cloud masses.
		/// [JP] ワールド座標から形状ノイズの領域へのスケール。小さいほど雲の塊が大きくなる。
		Float noiseScale_ = 0.0008f;

		/// [EN] Wind scroll speed in noise-domain units per second.
		/// [JP] 風によるスクロール速度（ノイズ領域の単位 / 秒）。
		Float windSpeed_ = 0.01f;

		/// [EN] Frame counter written by VolumetricCloudScapesRenderer::Prepare, not by the editor UI. It only rotates the raymarch start dither, and only while temporalJitter_ is non-zero.
		/// [JP] エディターの UI ではなく VolumetricCloudScapesRenderer::Prepare が書き込むフレームカウンター。レイマーチ開始位置のディザを回すことにだけ使い、それも temporalJitter_ が 0 でない間だけ効く。
		Uint32 frameIndex_ = 0;

		/// [EN] Cloud shape: 0 is stratus (a low, flat band), 1 is cumulus (tall and billowing).
		/// [JP] 雲の形。0 は層雲（低く平らな帯）、1 は積雲（縦に発達したもこもこ）。
		Float cloudType_ = 1.0f;

		/// [EN] 0 is fair weather, 1 is rain clouds: darker, denser, with more coverage.
		/// [JP] 0 は晴天、1 は雨雲（暗く、濃く、雲量が多い）。
		Float rain_ = 0.0f;

		/// [EN] Detail-noise domain scale relative to the shape-noise scale.
		/// [JP] 形状ノイズに対する、ディテールノイズの領域のスケール倍率。
		Float detailScale_ = 8.0f;

		/// [EN] Zenith color of the procedural sky, used only when no skymap is bound.
		/// [JP] プロシージャル空の天頂の色。スカイマップが無いときだけ使う。
		Float skyZenithColor_[3] = { 0.15f, 0.35f, 0.75f };

		/// [EN] Angular radius of the procedural sky's sun disc.
		/// [JP] プロシージャル空の太陽円盤の視半径。
		Float sunSize_ = 0.03f;

		/// [EN] Horizon color of the procedural sky.
		/// [JP] プロシージャル空の地平線の色。
		Float skyHorizonColor_[3] = { 0.65f, 0.75f, 0.9f };

		/// [EN] Overall brightness of the procedural sky.
		/// [JP] プロシージャル空全体の明るさ。
		Float skyBrightness_ = 1.0f;

		/// [EN] Color the procedural sky shows below the horizon.
		/// [JP] プロシージャル空が地平線より下に表示する地面の色。
		Float groundColor_[3] = { 0.25f, 0.22f, 0.2f };

		/// [EN] 1 while the cloud and sky pass is on. Written by VolumetricCloudScapesRenderer::Prepare, not by the editor UI; the composite reads it to decide whether a background without a skymap gets the procedural sky.
		/// [JP] 雲と空のパスが有効な間 1。エディターの UI ではなく VolumetricCloudScapesRenderer::Prepare が書き込む。合成時に、スカイマップの無い背景へプロシージャル空を描くかの判定に使う。
		Uint32 proceduralSkyEnabled_ = 0;

		/// [EN] Planet radius in world units. The cloud layer is a shell around a sphere of this radius, so it curves down and meets the horizon instead of running to infinity as a flat slab; larger values are flatter.
		/// [JP] 惑星の半径（ワールド単位）。雲層をこの半径の球殻として扱うため、平らな板のように無限に続かず、遠方で下がって地平線に収束する。大きいほど平らになる。
		Float planetRadius_ = 6360000.0f;

		/// [EN] Far limit of the cloud raymarch. Alpha fades out toward it, so the cut is invisible.
		/// [JP] 雲のレイマーチの遠方の上限。上限へ向けて alpha をフェードするため、切れ目は見えない。
		Float maxMarchDistance_ = 120000.0f;

		/// [EN] Distance at which the step length starts growing and detail noise starts fading, standing in for mipmaps the noise volumes do not have.
		/// [JP] ステップ長が伸び始め、ディテールノイズがフェードし始める距離。ノイズボリュームに無いミップマップの代わりになる。
		Float lodDistance_ = 25000.0f;

		/// [EN] Aerial perspective: the 1/e distance over which distant clouds fade toward the horizon color.
		/// [JP] 空気遠近。遠くの雲が地平線の色へ寄っていく減衰の 1/e 距離。
		Float aerialDensity_ = 0.000012f;

		/// [EN] Frequency of the weather field relative to noiseScale_; smaller values make larger cloud masses.
		/// [JP] noiseScale_ に対する weather フィールドの周波数。小さいほど雲の塊が大きくなる。
		Float weatherScale_ = 0.06f;

		/// [EN] How strongly the weather field varies the coverage, 0 to 1; 0 keeps the coverage uniform across the sky.
		/// [JP] weather フィールドが雲量をどれだけ変化させるか（0 から 1）。0 なら空全体で雲量が一定になる。
		Float weatherAmount_ = 0.6f;

		/// [EN] How much the detail noise erodes the cloud outline.
		/// [JP] ディテールノイズが雲の輪郭を削る量。
		Float detailStrength_ = 0.35f;

		/// [EN] Detail scroll speed relative to windSpeed_. The detail volume moves by its own offset; scrolling it by the shape offset times detailScale_ would slide the detail across the cloud body at detailScale_ times the wind speed, which reads as a boiling surface.
		/// [JP] windSpeed_ に対するディテールのスクロール速度。ディテールは独自のオフセットで動かす。形状のオフセットを detailScale_ 倍して使うと、ディテールが雲の本体に対して風の detailScale_ 倍の速さで滑り、表面が沸騰しているように見える。
		Float detailWindSpeed_ = 0.3f;

		/// [EN] Anisotropy of the backward lobe (positive; the shader negates it). A single lobe gives either the silver lining or the bright down-sun face, never both.
		/// [JP] 後方ローブの異方性（正の値。シェーダー側で符号を反転する）。単一のローブでは、逆光の縁取りと順光の明るい面のどちらか一方しか出せない。
		Float phaseBackward_ = 0.35f;

		/// [EN] Blend weight between the forward and backward lobes.
		/// [JP] 前方ローブと後方ローブのブレンド率。
		Float phaseBlend_ = 0.4f;

		/// [EN] Beer-powder strength, 0 to 1. Darkening the sunlit surface is what makes a cloud's shape readable instead of a flat saturated blob.
		/// [JP] Beer-powder の強さ（0 から 1）。日の当たる面を暗くすることで、白く飛んだ塊ではなく雲の形が読めるようになる。
		Float powderStrength_ = 1.0f;

		/// [EN] Multiplier of the sky-gradient ambient light, which keeps cloud undersides from going black.
		/// [JP] 空のグラデーションによる環境光の倍率。雲の底が真っ黒に潰れないようにする。
		Float ambientStrength_ = 0.4f;

		/// [EN] Number of multiple-scattering octaves, which fill in cloud interiors that single scattering cannot light. A thick cloud scatters a photon dozens of times with almost no absorption, so it behaves as a near-Lambertian reflector of albedo about 0.9 (about 0.286*E), while a single backscatter lobe gives only about 0.11*E; the octave sum closes that gap.
		/// [JP] 多重散乱のオクターブ数。単一散乱では照らせない雲の内部を埋める。厚い雲は光子をほぼ吸収せずに数十回散乱させるため、アルベド約 0.9 のほぼランバート反射体（約 0.286*E）として振る舞うが、単一の後方散乱ローブは約 0.11*E しかない。その差をオクターブの総和で埋める。
		Uint32 multiScatterOctaves_ = 5;

		/// [EN] Per-octave scale of the extinction.
		/// [JP] オクターブごとの消散の減衰率。
		Float multiScatterAttenuation_ = 0.5f;

		/// [EN] Per-octave scale of the contribution. With 5 octaves, 0.75 sums to 3.05, which puts a sunlit cloud face at about 0.33*E, matching the 0.318*E of a sunlit white Lambertian surface in this engine.
		/// [JP] オクターブごとの寄与の減衰率。5 オクターブで 0.75 なら総和は 3.05 となり、日の当たる雲の面は約 0.33*E になって、このエンジンで日の当たる白いランバート面の 0.318*E とほぼ一致する。
		Float multiScatterContribution_ = 0.75f;

		/// [EN] Per-octave scale of the phase eccentricity. The octaves only add arithmetic; the optical depth is marched once and reused.
		/// [JP] オクターブごとの位相の偏心の減衰率。オクターブで増えるのは演算だけで、光学的深さのマーチは 1 回で使い回す。
		Float multiScatterEccentricity_ = 0.5f;

		/// [EN] 0 gives a frame-independent dither (stable, no crawling grain); 1 rotates it fully every frame, for use behind TAA or DLSS.
		/// [JP] 0 はフレームに依存しないディザ（安定し、粒が這わない）。1 は毎フレーム完全に変化させ、TAA や DLSS で解決する前提。
		Float temporalJitter_ = 0.0f;

		/// [EN] Pads the last row to a full 16-byte register.
		/// [JP] 最後の行を 16 バイトのレジスタ 1 つ分に揃える詰め物。
		Float cloudPadding0_ = 0.0f;
		Float cloudPadding1_ = 0.0f;
		Float cloudPadding2_ = 0.0f;

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
			archive.TryField("cloudBottom", cloudBottom_);
			archive.TryField("cloudTop", cloudTop_);
			archive.TryField("coverage", coverage_);
			archive.TryField("densityScale", densityScale_);
			archive.TryField("cloudAlbedo", cloudAlbedo_);
			archive.TryField("scatteringG", scatteringG_);
			archive.TryField("stepCount", stepCount_);
			archive.TryField("lightStepCount", lightStepCount_);
			archive.TryField("noiseScale", noiseScale_);
			archive.TryField("windSpeed", windSpeed_);
			archive.TryField("cloudType", cloudType_);
			archive.TryField("rain", rain_);
			archive.TryField("detailScale", detailScale_);
			archive.TryField("skyZenithColor", skyZenithColor_);
			archive.TryField("sunSize", sunSize_);
			archive.TryField("skyHorizonColor", skyHorizonColor_);
			archive.TryField("skyBrightness", skyBrightness_);
			archive.TryField("groundColor", groundColor_);
			archive.TryField("planetRadius", planetRadius_);
			archive.TryField("maxMarchDistance", maxMarchDistance_);
			archive.TryField("lodDistance", lodDistance_);
			archive.TryField("aerialDensity", aerialDensity_);
			archive.TryField("weatherScale", weatherScale_);
			archive.TryField("weatherAmount", weatherAmount_);
			archive.TryField("detailStrength", detailStrength_);
			archive.TryField("detailWindSpeed", detailWindSpeed_);
			archive.TryField("phaseBackward", phaseBackward_);
			archive.TryField("phaseBlend", phaseBlend_);
			archive.TryField("powderStrength", powderStrength_);
			archive.TryField("ambientStrength", ambientStrength_);
			archive.TryField("multiScatterOctaves", multiScatterOctaves_);
			archive.TryField("multiScatterAttenuation", multiScatterAttenuation_);
			archive.TryField("multiScatterContribution", multiScatterContribution_);
			archive.TryField("multiScatterEccentricity", multiScatterEccentricity_);
			archive.TryField("temporalJitter", temporalJitter_);
		}
	};

	/// [EN] The HLSL side is 12 rows of four scalars; nothing checks the layout at runtime (a shifted field would silently corrupt every field after it), so the size is asserted here.
	/// [JP] HLSL 側は 4 スカラーの行が 12 行。実行時にはレイアウトを検証できず、ずれたフィールド以降がすべて化けた値になるため、ここでサイズを静的に検証する。
	static_assert(sizeof(VolumetricCloudScapesRayConstantBuffer) == 12 * 4 * sizeof(Float), "VolumetricCloudScapesRayConstantBuffer が VolumetricCloudScapes.hlsli とバイト単位で一致していません");

	/**
	* [EN]
	* Runs the cloud pass (VolumetricCloudScapesRT.hlsl: a screen-space
	* raymarch through a procedural density field, sky pixels only) into an
	* RGBA16F texture - rgb is in-scattered radiance, a is coverage - and
	* leaves it in shader-resource state for DeferredLightingPS.hlsl to
	* composite over the sky. It needs no TLAS, since it is a pure raymarch,
	* and like SubsurfaceScatteringRenderer it has no denoiser and no
	* per-view chain: one texture is written and consumed within each view's
	* flush. When the pass is off the texture is cleared to 0 (no clouds).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 雲のパス（VolumetricCloudScapesRT.hlsl。プロシージャルな密度場を
	* スクリーン空間でレイマーチする。空のピクセルのみ）を RGBA16F テクスチャ
	* （rgb は内散乱の放射輝度、a はカバレッジ）へディスパッチし、
	* DeferredLightingPS.hlsl が空の上に合成できるよう、シェーダーリソース
	* 状態にしておく。純粋なレイマーチなので TLAS は不要。
	* SubsurfaceScatteringRenderer と同じくデノイザもビューごとのチェーンも
	* 持たず、各ビューの Flush の中で書いて読む 1 枚のテクスチャで足りる。
	* パスが無効ならテクスチャを 0（雲なし）でクリアする。
	*/
	class VolumetricCloudScapesRenderer
	{
	public:
		/**
		* [EN]
		* Binds the shared root signature and pipeline-state cache that the
		* cloud shaders compile into.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 雲のシェーダーのコンパイル先となる、共有のルートシグネチャと
		* パイプラインステートキャッシュを関連付ける。
		*/
		VolumetricCloudScapesRenderer(RootSignature& rootSignature, PipelineStateObject& pipelineStateObject);

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
		~VolumetricCloudScapesRenderer() = default;

		/**
		* [EN]
		* Compiles the shaders, creates the tuning constant buffer and the two
		* fixed-size noise volumes, and allocates the cloud texture for a
		* width x height render size.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シェーダーをコンパイルし、調整値の定数バッファと固定サイズのノイズ
		* ボリューム 2 つを作成して、width x height のレンダーサイズに合わせた
		* 雲テクスチャを確保する。
		*/
		void Create(ID3D12Device* device, BindlessHeap* bindlessHeap, ShaderCache& shaderCache, ConstantIndicesSystem& constantIndicesSystem, ShaderResourceIndicesSystem& shaderResourceIndicesSystem, UnorderedAccessIndicesSystem& unorderedAccessIndicesSystem, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Recreates the cloud texture for a new render size. The noise
		* volumes do not depend on the size and are kept.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新しいレンダーサイズに合わせて雲テクスチャを作り直す。ノイズ
		* ボリュームはサイズに依存しないため、そのまま残す。
		*/
		void Resize(ID3D12Device* device, Uint32 width, Uint32 height);

		/**
		* [EN]
		* Uploads the tuning values together with the frame counter and the
		* enabled flag, and publishes this pass's bindless indices to the
		* index systems. It records no GPU work, and must run before the index
		* systems upload this frame's indices.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フレームカウンターと有効フラグを入れた調整値をアップロードし、この
		* パスの bindless インデックスを各インデックスシステムへ公開する。
		* GPU 処理は記録せず、各インデックスシステムが今フレームのインデックスを
		* アップロードする前に呼び出す。
		*/
		void Prepare(const VolumetricCloudScapesRayConstantBuffer& settings, Bool enabled);

		/**
		* [EN]
		* Records the pass: bakes the noise volumes on the first run, then
		* raymarches the clouds into the cloud texture, or clears it to 0 when
		* the pass is off, and leaves the texture in shader-resource state.
		* The G-Buffer depth must already be written, since it tells sky pixels
		* apart.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パスを記録する。初回はノイズボリュームを焼き込み、その後雲テクスチャへ
		* 雲をレイマーチするか、パスが無効なら 0 でクリアして、テクスチャを
		* シェーダーリソース状態で終える。空のピクセルを見分けるため、G-Buffer
		* の深度が書き込み済みであることが前提。
		*/
		void Dispatch(D3D12CommandList* cmdList, ID3D12DescriptorHeap* heap, const RootAddresses& addresses);

	private:
		/**
		* [EN]
		* Creates the width_ x height_ cloud texture and its bindless and
		* clear descriptors.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* width_ x height_ の雲テクスチャと、その bindless ディスクリプタおよび
		* クリア用ディスクリプタを作成する。
		*/
		void Allocate(ID3D12Device* device);

		/**
		* [EN]
		* Frees the cloud texture's bindless descriptors and defers the
		* texture's destruction until the GPU has finished with it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 雲テクスチャの bindless ディスクリプタを解放し、テクスチャ自体の破棄は
		* GPU が使い終えるまで遅延させる。
		*/
		void Release();

	private:
		/// [EN] The cloud texture is 1/resolutionDivisor_ of the render size on each axis and is upsampled bilinearly by DeferredLightingPS.hlsl. Clouds are a low-frequency signal, so a divisor of N cuts the cost by N^2 at the price of softer edges; 1 renders at full resolution.
		/// [JP] 雲テクスチャは各軸でレンダーサイズの 1/resolutionDivisor_ とし、DeferredLightingPS.hlsl がバイリニアで拡大する。雲は低周波な信号なので、N で割るとコストは N^2 分の 1 になり、代償は輪郭が柔らかくなることだけ。1 ならフル解像度で描く。
		SC_CONST Uint32 resolutionDivisor_ = 1;

		/// [EN] Edge length in texels of the Perlin-Worley shape-noise volume.
		/// [JP] Perlin-Worley 形状ノイズボリュームの一辺のテクセル数。
		SC_CONST Uint32 shapeNoiseSize_ = 128;

		/// [EN] Edge length in texels of the Worley detail-noise volume.
		/// [JP] Worley ディテールノイズボリュームの一辺のテクセル数。
		SC_CONST Uint32 detailNoiseSize_ = 64;

		/// [EN] Raymarch and noise-bake compute shaders of the cloud pass.
		/// [JP] 雲パスのレイマーチ用とノイズ焼き込み用のコンピュートシェーダー。
		VolumetricCloudScapesShader cloudShader_;

		/// [EN] GPU copy of the tuning values, read through its bindless index.
		/// [JP] 調整値の GPU 側コピー。bindless インデックス経由で読まれる。
		ResourcePtr<StaticConstantBuffer<VolumetricCloudScapesRayConstantBuffer>> tuningBuffer_;

		/// [EN] Cloud texture: rgb is in-scattered radiance, a is coverage.
		/// [JP] 雲テクスチャ。rgb は内散乱の放射輝度、a はカバレッジ。
		Microsoft::WRL::ComPtr<ID3D12Resource> cloudResource_;

		/// [EN] Resource state the cloud texture is currently in, tracked to issue only the barriers that are needed.
		/// [JP] 雲テクスチャの現在のリソース状態。必要なバリアだけを発行するために追跡する。
		D3D12_RESOURCE_STATES cloudState_ = D3D12_RESOURCE_STATE_COMMON;

		/// [EN] Bindless index of the cloud texture's write view.
		/// [JP] 雲テクスチャの書き込み用ビューの bindless インデックス。
		Uint32 cloudUnorderedAccessViewIndex_ = 0;

		/// [EN] Bindless index of the cloud texture's read view.
		/// [JP] 雲テクスチャの読み取り用ビューの bindless インデックス。
		Uint32 cloudShaderResourceViewIndex_ = 0;

		/// [EN] Tileable Perlin-Worley shape-noise volume, baked once and then sampled with wrapping. It is R16_FLOAT because the coverage remap in CloudDensity divides by coverage_, which would amplify 8-bit quantization steps into terraced cloud edges.
		/// [JP] タイル可能な Perlin-Worley 形状ノイズボリューム。1 度だけ焼き込み、以後は wrap でサンプルする。CloudDensity の雲量リマップは coverage_ で割るため、8 ビットの量子化幅が増幅されて雲の縁が階段状になる。そのため R16_FLOAT にしている。
		Microsoft::WRL::ComPtr<ID3D12Resource> shapeNoiseResource_;

		/// [EN] Bindless indices of the shape-noise volume's write view (used by the bake) and read view (used by the raymarch).
		/// [JP] 形状ノイズボリュームの書き込み用ビュー（焼き込みで使う）と読み取り用ビュー（レイマーチで使う）の bindless インデックス。
		Uint32 shapeNoiseUnorderedAccessViewIndex_ = 0;
		Uint32 shapeNoiseShaderResourceViewIndex_ = 0;

		/// [EN] Tileable Worley detail-noise volume, baked and sampled like the shape noise.
		/// [JP] タイル可能な Worley ディテールノイズボリューム。形状ノイズと同じく焼き込んでサンプルする。
		Microsoft::WRL::ComPtr<ID3D12Resource> detailNoiseResource_;

		/// [EN] Bindless indices of the detail-noise volume's write and read views.
		/// [JP] ディテールノイズボリュームの書き込み用と読み取り用ビューの bindless インデックス。
		Uint32 detailNoiseUnorderedAccessViewIndex_ = 0;
		Uint32 detailNoiseShaderResourceViewIndex_ = 0;

		/// [EN] Whether the noise volumes have been baked. Baking needs a command list, so it happens on the first Dispatch rather than in Create.
		/// [JP] ノイズボリュームを焼き込み済みか。焼き込みにはコマンドリストが要るため、Create ではなく最初の Dispatch で行う。
		Bool noiseBaked_ = false;

		/// [EN] Non-shader-visible heap holding the CPU-side write view that ClearUnorderedAccessViewFloat requires alongside the shader-visible one.
		/// [JP] ClearUnorderedAccessViewFloat がシェーダー可視のビューと併せて要求する、CPU 側の書き込み用ビューを置く非シェーダー可視ヒープ。
		DescriptorHeap clearHeap_;

		/// [EN] Index of the cloud texture's write view inside clearHeap_.
		/// [JP] clearHeap_ 内での雲テクスチャの書き込み用ビューのインデックス。
		Uint32 clearIndex_ = 0;

		/// [EN] Bindless heap that owns this pass's descriptors.
		/// [JP] このパスのディスクリプタを所有する bindless ヒープ。
		BindlessHeap* bindlessHeap_ = nullptr;

		/// [EN] Index systems that receive this pass's bindless indices in Prepare.
		/// [JP] Prepare でこのパスの bindless インデックスを受け取るインデックスシステム。
		ConstantIndicesSystem* constantIndicesSystem_ = nullptr;
		ShaderResourceIndicesSystem* shaderResourceIndicesSystem_ = nullptr;
		UnorderedAccessIndicesSystem* unorderedAccessIndicesSystem_ = nullptr;

		/// [EN] Size of the cloud texture: the render size divided by resolutionDivisor_, rounded up.
		/// [JP] 雲テクスチャのサイズ。レンダーサイズを resolutionDivisor_ で割って切り上げたもの。
		Uint32 width_ = 0;
		Uint32 height_ = 0;

		/// [EN] Frame counter copied into frameIndex_ of the uploaded tuning values.
		/// [JP] アップロードする調整値の frameIndex_ へ写すフレームカウンター。
		Uint32 frameIndex_ = 0;

		/// [EN] Whether the pass renders this frame.
		/// [JP] 今フレームにパスを描画するか。
		Bool enabled_ = false;

		/// [EN] Whether the missing-pipeline warning has been logged, so it appears once rather than every frame.
		/// [JP] パイプライン欠如の警告を出力済みか。毎フレームではなく 1 度だけ出すために使う。
		Bool pipelineStateMissingLogged_ = false;
	};
}
