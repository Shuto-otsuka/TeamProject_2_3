#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Resource/Config/BootConfig.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <GraphicsEngine/D3D12/SwapChain/SwapChain.h>
#include <GraphicsEngine/DLSS/DlssManager.h>
#include <GraphicsEngine/Quality/Upscale.h>
#include <GraphicsEngine/Renderer/Renderer.h>
#include <GraphicsEngine/Renderer/ViewMode.h>
#include <GraphicsEngine/Shader/ShaderCache.h>
#include <GraphicsEngine/Shape/Screen/BootScreen.h>
#include <GraphicsEngine/Shape/Screen/FadeScreen.h>
#include <GraphicsEngine/Shape/Screen/LetterScreen.h>
#include <GraphicsEngine/Shape/Screen/SplashScreen.h>
#include <GraphicsEngine/System/MovieSystem.h>
#include <GraphicsEngine/System/SceneSystem.h>
#include <GraphicsEngine/Texture/Compression/BC7CompressShader.h>

namespace SeedCore
{
	struct LoaderSystem;
	struct RaytracingContext;
	struct SceneConstantBuffer;

	class AvatarMesh;
	class CameraSystem;
	class Entity;
	class CanvasCamera;
	class EditorCamera;
	class GameTimer;
	class GpuProfiler;
	class PreviewCamera;
	class ResourceCache;
	class SplashSystem;
	class World;
	class WorldTimer;

	/**
	* [EN]
	* Coordinates the graphics frame lifecycle. Graphics owns the D3D12
	* context and presentation resources, prepares shared scene data once
	* per frame, and forwards each view's rendering request to Renderer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフィックスフレームのライフサイクルを統括する。D3D12 コンテキストと
	* 表示用リソースを所有し、共有シーンデータをフレームごとに一度準備して、
	* 各ビューの描画要求を Renderer へ転送する。
	*/
	class SEEDCORE_API Graphics
	{
	public:
		/**
		* [EN]
		* Creates an empty graphics coordinator. Initialize must be called
		* before any frame or rendering operation.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 空のグラフィックス統括オブジェクトを構築する。フレーム処理や描画を
		* 行う前に Initialize を呼び出す必要がある。
		*/
		Graphics() = default;

		/**
		* [EN]
		* Destroys the coordinator. Finalize is responsible for explicitly
		* releasing GPU-facing resources before destruction.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 統括オブジェクトを破棄する。GPU を扱うリソースは、破棄前に
		* Finalize で明示的に解放する責務を持つ。
		*/
		~Graphics() = default;

		/**
		* [EN]
		* Initializes the device, swap chain, renderer, screen overlays, and
		* DLSS services for hwnd and its initial output size. bootConfig
		* supplies the boot screen's images, which are uploaded once here.
		* Returns false if any required service fails to initialize.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* hwnd と初期出力サイズに対して、デバイス、スワップチェイン、
		* Renderer、画面オーバーレイ、および DLSS サービスを初期化する。
		* bootConfig はブート画面の画像を与え、ここで一度だけアップロードする。
		* 必要なサービスのいずれかの初期化に失敗した場合は false を返す。
		*/
		Bool Initialize(HWND hwnd, Uint32 width, Uint32 height, const BootConfig& bootConfig);

		/**
		* [EN]
		* Waits for submitted GPU work, then releases resources in the order
		* required by their device and descriptor dependencies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 投入済み GPU 処理を待機してから、デバイスとディスクリプタの依存関係に
		* 必要な順序でリソースを解放する。
		*/
		void Finalize();

		/**
		* [EN]
		* Blocks until the direct, copy, and compute queues have completed
		* every command submitted before this call.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この呼び出し以前に direct、copy、compute キューへ投入された全コマンドが
		* 完了するまでブロックする。
		*/
		void Wait();

	public:
		/**
		* [EN]
		* Resizes renderer-owned render targets for the output size and the
		* render scale selected by upscaleMode.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 出力サイズと upscaleMode が選ぶレンダー倍率に合わせて、Renderer が
		* 所有するレンダーターゲットをリサイズする。
		*/
		void ResizeRenderTarget(Uint32 outputWidth, Uint32 outputHeight, UpscaleMode upscaleMode);

		/**
		* [EN]
		* Recreates the swap chain buffers for a new presentation size.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 新しい表示サイズに合わせて、スワップチェインバッファを再作成する。
		*/
		void ResizeSwapChain(Uint32 width, Uint32 height);

	public:
		/**
		* [EN]
		* Starts a presentation frame and transitions the current back buffer
		* from present state to render-target state.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 表示フレームを開始し、現在のバックバッファを present 状態から
		* render-target 状態へ遷移させる。
		*/
		void Begin();

		/**
		* [EN]
		* Binds the current swap chain back buffer as the active render target.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在のスワップチェインバックバッファを、アクティブなレンダーターゲット
		* としてバインドする。
		*/
		void Bind();

		/**
		* [EN]
		* Finishes a presentation frame, restores the back buffer to present
		* state, and advances deferred bindless-resource retirement.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 表示フレームを終了し、バックバッファを present 状態へ戻して、
		* 遅延した bindless リソース回収を進める。
		*/
		void End();

	public:
		/**
		* [EN]
		* Renders the editor view using editorCamera and optionally highlights
		* selected entities.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* editorCamera を用いてエディタービューを描画し、必要に応じて選択中の
		* エンティティをハイライトする。
		*/
		void EditorRender(WorldTimer& timer, const EditorCamera& editorCamera, LoaderSystem& loaderSystem, ResourceCache& resourceCache, World& world, ViewMode viewMode, std::span<const Entity> selectedEntities = {});

		/**
		* [EN]
		* Renders the game view from the active camera managed by cameraSystem.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* cameraSystem が管理するアクティブカメラからゲームビューを描画する。
		*/
		void GameRender(GameTimer& timer, CameraSystem& cameraSystem, LoaderSystem& loaderSystem, ResourceCache& resourceCache, World& world);

		/**
		* [EN]
		* Renders the canvas view using its dedicated screen-space camera.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 専用のスクリーンスペースカメラを用いてキャンバスビューを描画する。
		*/
		void CanvasRender(WorldTimer& timer, const CanvasCamera& canvasCamera, LoaderSystem& loaderSystem, ResourceCache& resourceCache, World& world);

		/**
		* [EN]
		* Renders an animation timeline preview for the selected mesh and animation assets.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 選択中のメッシュおよびアニメーションアセットのタイムラインプレビューを描画する。
		*/
		void TimelineRender(WorldTimer& timer, const PreviewCamera& timelineCamera, LoaderSystem& loaderSystem, ResourceCache& resourceCache, Uint32 meshAssetId, Uint32 animationAssetId, Float time, const Matrix& worldMatrix);

		/**
		* [EN]
		* Renders a model-transform preview for the selected mesh and animation assets.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 選択中のメッシュおよびアニメーションアセットのモデル変形プレビューを描画する。
		*/
		void ModelTransformRender(WorldTimer& timer, const PreviewCamera& modelTransformCamera, LoaderSystem& loaderSystem, ResourceCache& resourceCache, Uint32 meshAssetId, Uint32 animationAssetId, Float time, const Matrix& worldMatrix);

		/**
		* [EN]
		* Renders a material preview for the selected mesh and surface assets.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 選択中のメッシュおよびサーフェスアセットのマテリアルプレビューを描画する。
		*/
		void MaterialRender(WorldTimer& timer, const PreviewCamera& materialCamera, LoaderSystem& loaderSystem, ResourceCache& resourceCache, Uint32 meshAssetId, Uint32 surfaceAssetId, const Matrix& worldMatrix);

		/**
		* [EN]
		* Renders a skeleton-controller preview and its selected node state.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スケルトンコントローラープレビューと、選択中ノードの状態を描画する。
		*/
		void SkeletonControllerRender(WorldTimer& timer, const PreviewCamera& skeletonControllerCamera, LoaderSystem& loaderSystem, ResourceCache& resourceCache, Uint32 meshAssetId, Uint32 animationAssetId, Float time, const Matrix& worldMatrix, Int selectedNodeIndex);

		/**
		* [EN]
		* Renders an avatar preview with its supplied skinning and region-texture data.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定されたスキニング情報とリージョンテクスチャ情報でアバタープレビューを描画する。
		*/
		void AvatarRender(WorldTimer& timer, const PreviewCamera& avatarCamera, const AvatarMesh& mesh, Uint32 boneCount, const Matrix& worldMatrix, std::span<const Uint32> regionTextureIndices);

	public:
		/**
		* [EN]
		* Draws and presents the splash or boot screen for the current splash
		* phase. It runs a whole presentation frame by itself (Begin, draw, End,
		* Present), so it is called in place of the normal frame, never
		* between Begin and End.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在のスプラッシュフェーズに対応するスプラッシュ画面またはブート画面を
		* 描画して表示する。表示フレーム全体（Begin、描画、End、Present）を
		* 単独で行うため、通常フレームの代わりに呼び出し、Begin と End の間では
		* 呼び出さない。
		*/
		void DrawSplashScreen(const SplashSystem& splashSystem);

		/**
		* [EN]
		* Draws the game display resource letterboxed onto the back buffer.
		* Called inside a normal frame, after GameRender and before End.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲーム表示リソースをレターボックス付きでバックバッファへ描画する。
		* 通常フレーム内で、GameRender の後、End の前に呼び出す。
		*/
		void DrawLetterScreen();

	public:
		/**
		* [EN]
		* Applies raytracing settings to the renderer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レイトレーシング設定を Renderer へ適用する。
		*/
		void Raytracing(const RaytracingContext& settings);

		/**
		* [EN]
		* Selects the renderer's upscaling path and DLSS ray-reconstruction mode.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Renderer のアップスケール経路と DLSS レイ再構成モードを選択する。
		*/
		void Upscale(Bool dlssRayReconstructionEnabled, UpscaleMode upscaleMode);

		/**
		* [EN]
		* Configures NVIDIA Reflex and its optional boost mode.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* NVIDIA Reflex と任意のブーストモードを設定する。
		*/
		void Reflex(Bool enable, Bool useBoost);

		/**
		* [EN]
		* Configures DLSS DeepDVC enhancement and its tuning values.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* DLSS DeepDVC 強調処理とその調整値を設定する。
		*/
		void DeepDVC(Bool enable, Float intensity, Float saturationBoost);

		/**
		* [EN]
		* Enables or disables DLSS frame generation, recreating presentation resources when needed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 必要に応じて表示用リソースを再作成し、DLSS フレーム生成を有効または無効にする。
		*/
		void FrameGeneration(Bool enable);

		/**
		* [EN]
		* Sets whether swap chain presentation waits for vertical synchronization.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スワップチェインの表示で垂直同期を待機するか設定する。
		*/
		void VerticalSync(Bool vsync);

		/**
		* [EN]
		* Returns whether vertical synchronization is enabled for presentation.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 表示時の垂直同期が有効か返す。
		*/
		Bool VerticalSync()const;

	public:
		/**
		* [EN]
		* Returns the D3D12 device and command-queue context. Like every
		* service getter below, the reference stays valid until Finalize.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* D3D12 デバイスおよびコマンドキューのコンテキストを返す。以下の
		* サービス取得関数と同様に、参照は Finalize まで有効である。
		*/
		[[nodiscard]] D3D12Context& GetContext()const;

		/**
		* [EN]
		* Returns the presentation swap chain.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 表示用スワップチェインを返す。
		*/
		[[nodiscard]] SwapChain& GetSwapChain()const;

		/**
		* [EN]
		* Returns the shared bindless descriptor allocator.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 共有 bindless ディスクリプタアロケーターを返す。
		*/
		[[nodiscard]] BindlessHeap& GetBindlessHeap()const;

		/**
		* [EN]
		* Returns the texture-streaming BC7 compression pass.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* テクスチャストリーミング用 BC7 圧縮パスを返す。
		*/
		[[nodiscard]] BC7CompressShader& GetBC7CompressShader()const;

		/**
		* [EN]
		* Returns renderer-owned GPU timing data.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Renderer が所有する GPU タイミングデータを返す。
		*/
		[[nodiscard]] const GpuProfiler& GetGpuProfiler()const;

	public:
		/**
		* [EN]
		* Returns the shader-visible handle of the editor view display texture.
		* This and the other display handles below are bindless-heap handles
		* that the editor UI draws as images.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エディタービュー表示テクスチャのシェーダー可視ハンドルを返す。
		* これと以下の表示ハンドルは bindless ヒープ上のハンドルで、
		* エディター UI が画像として描画する。
		*/
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE EditorDisplayGPUHandle()const;

		/**
		* [EN]
		* Returns the game view display texture.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲームビュー表示テクスチャを返す。
		*/
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GameDisplayGPUHandle()const;

		/**
		* [EN]
		* Returns the canvas view display texture.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キャンバスビュー表示テクスチャを返す。
		*/
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE CanvasDisplayGPUHandle()const;

		/**
		* [EN]
		* Returns the timeline preview display texture.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* タイムラインプレビュー表示テクスチャを返す。
		*/
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE TimelineDisplayGPUHandle()const;

		/**
		* [EN]
		* Returns the model-transform preview display texture.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* モデル変形プレビュー表示テクスチャを返す。
		*/
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE ModelTransformDisplayGPUHandle()const;

		/**
		* [EN]
		* Returns the material preview display texture.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* マテリアルプレビュー表示テクスチャを返す。
		*/
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE MaterialDisplayGPUHandle()const;

		/**
		* [EN]
		* Returns the skeleton-controller preview display texture.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スケルトンコントローラープレビュー表示テクスチャを返す。
		*/
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE SkeletonControllerDisplayGPUHandle()const;

		/**
		* [EN]
		* Returns the avatar preview display texture.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アバタープレビュー表示テクスチャを返す。
		*/
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE AvatarDisplayGPUHandle()const;

	private:
		/**
		* [EN]
		* Performs per-frame shared resource updates before the first view
		* renders. prepared_ prevents later views from performing the work again.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 最初のビューを描画する前に、フレーム共通のリソース更新を行う。
		* prepared_ により、後続ビューでの処理重複を防ぐ。
		*/
		void Prepare(Float deltaTime, LoaderSystem& loaderSystem, ResourceCache& resourceCache, World& world, std::span<const Entity> selectedEntities, const SceneConstantBuffer& streamingScene);

	private:
		/// [EN] Current swap chain output width in pixels.
		/// [JP] 現在のスワップチェイン出力幅（ピクセル）。
		Uint32 width_ = static_cast<Uint32>(ScResolution::SC_HD.Width);

		/// [EN] Current swap chain output height in pixels.
		/// [JP] 現在のスワップチェイン出力高さ（ピクセル）。
		Uint32 height_ = static_cast<Uint32>(ScResolution::SC_HD.Height);

		/// [EN] Internal renderer width after applying the selected render scale.
		/// [JP] 選択されたレンダー倍率を適用した後の内部 Renderer 幅。
		Uint32 nativeWidth_ = static_cast<Uint32>(ScResolution::SC_HD.Width);

		/// [EN] Internal renderer height after applying the selected render scale.
		/// [JP] 選択されたレンダー倍率を適用した後の内部 Renderer 高さ。
		Uint32 nativeHeight_ = static_cast<Uint32>(ScResolution::SC_HD.Height);

		/// [EN] True after shared streaming and renderer preparation has run for the current frame.
		/// [JP] 現在のフレームで共有ストリーミング処理と Renderer 準備が完了した後に true となる。
		Bool prepared_ = false;

		/// [EN] Owns the D3D12 device, queues, and command-list frame lifecycle.
		/// [JP] D3D12 デバイス、キュー、およびコマンドリストのフレームライフサイクルを所有する。
		ResourcePtr<D3D12Context> context_;

		/// [EN] Owns the presentation back buffers and swap chain state.
		/// [JP] 表示用バックバッファとスワップチェイン状態を所有する。
		ResourcePtr<SwapChain> swapChain_;

		/// [EN] Caches shader objects shared by renderer and utility passes.
		/// [JP] Renderer とユーティリティパスで共有するシェーダーオブジェクトをキャッシュする。
		ResourcePtr<ShaderCache> shaderCache_;

		/// [EN] Performs GPU BC7 compression for streamed texture data.
		/// [JP] ストリーミングテクスチャデータの GPU BC7 圧縮を行う。
		ResourcePtr<BC7CompressShader> bc7CompressShader_;

		/// [EN] Owns bindless descriptors and their deferred reuse schedule.
		/// [JP] bindless ディスクリプタと、その遅延再利用スケジュールを所有する。
		ResourcePtr<BindlessHeap> bindlessHeap_;

		/// [EN] Owns DLSS, Reflex, and frame-generation integration state.
		/// [JP] DLSS、Reflex、フレーム生成との統合状態を所有する。
		ResourcePtr<DlssManager> dlssManager_;

		/// [EN] Owns the render passes and their view-specific frame buffers.
		/// [JP] レンダーパスとビュー固有のフレームバッファを所有する。
		ResourcePtr<Renderer> renderer_;

		/// [EN] Maintains the scene constants and GPU data for the editor view.
		/// [JP] エディタービュー用のシーン定数と GPU データを管理する。
		ResourcePtr<SceneSystem> editorSceneSystem_;

		/// [EN] Maintains the scene constants and GPU data for the game view.
		/// [JP] ゲームビュー用のシーン定数と GPU データを管理する。
		ResourcePtr<SceneSystem> gameSceneSystem_;

		/// [EN] Maintains the scene constants and GPU data for the canvas view.
		/// [JP] キャンバスビュー用のシーン定数と GPU データを管理する。
		ResourcePtr<SceneSystem> canvasSceneSystem_;

		/// [EN] Updates movie resources before rendering consumes their current frame.
		/// [JP] 描画が現在フレームを消費する前に、ムービーリソースを更新する。
		MovieSystem movieSystem_;

		/// [EN] Draws phase-specific logos and splash imagery.
		/// [JP] フェーズ固有のロゴとスプラッシュ画像を描画する。
		SplashScreen splashScreen_;

		/// [EN] Draws the configurable boot progress screen.
		/// [JP] 設定可能なブート進行画面を描画する。
		BootScreen bootScreen_;

		/// [EN] Draws the scene-transition fade overlay.
		/// [JP] シーン遷移用のフェードオーバーレイを描画する。
		FadeScreen fadeScreen_;

		/// [EN] Draws the game-display letterbox overlay.
		/// [JP] ゲーム表示用のレターボックスオーバーレイを描画する。
		LetterScreen letterScreen_;
	};
}
