#pragma once
#include <GraphicsEngine/Quality/Upscale.h>

namespace SeedCore
{
	enum class RenderViewMode :Uint32;
	enum class RenderViewOutput :Uint8;

	struct GameConfig;

	class CameraSystem;
	class CanvasCamera;
	class EditorCamera;
	class GameTimer;
	class GpuProfiler;
	class GraphicsDisplay;
	class GraphicsModule;
	class RenderDevice;
	class SplashSystem;
	class Window;
	class World;
	class WorldTimer;

	/**
	* [EN]
	* Presentation and editor settings Graphics applies every frame through
	* Configure. Rendering effects are not configured here; each effect's
	* Feature reads its own component.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Graphics が毎フレーム Configure で反映する、表示とエディターの設定。
	* 描画の効果の設定はここに入れず、各効果の Feature が自分のコンポーネント
	* から読む。
	*/
	struct GraphicsSettings
	{
		/// [EN] Width the editor and game views are displayed at, in pixels.
		/// [JP] エディターとゲームのビューを表示する幅（ピクセル）。
		Uint32 outputWidth_ = 0;

		/// [EN] Height the editor and game views are displayed at, in pixels.
		/// [JP] エディターとゲームのビューを表示する高さ（ピクセル）。
		Uint32 outputHeight_ = 0;

		/// [EN] Upscale method and mode; the mode's render scale sets the size the views are rendered at.
		/// [JP] アップスケールの方式とモード。モードの倍率が、ビューを描くサイズを決める。
		GraphicsUpscale upscale_;

		/// [EN] Whether presentation waits for vertical synchronization.
		/// [JP] 表示で垂直同期を待つか。
		Bool verticalSync_ = false;

		/// [EN] Whether DLSS frame generation is on. Changing it recreates the swap chain.
		/// [JP] DLSS のフレーム生成を使うか。変えるとスワップチェインを作り直す。
		Bool frameGeneration_ = false;

		/// [EN] Whether the shape debug display (camera frustums, light and audio ranges) covers every actor rather than only the selected ones.
		/// [JP] 形のデバッグ表示（カメラの視錐台、ライトと音の範囲）を、選択中だけでなく全アクターに出すか。
		Bool shapeVisible_ = false;
	};

	/**
	* [EN]
	* Coordinates the graphics frame lifecycle. Graphics owns the render
	* device, the presenter, the GPU processor, the asset module, the
	* updater and the scene, and calls them in frame order: it advances the per-frame
	* state that rendering reads (animation, constraints, materials and
	* other assets), collects the editor, game and canvas views during the
	* frame, and renders them all at once through SceneRenderer when the
	* frame ends.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* グラフィックスフレームのライフサイクルを統括する。描画デバイス、
	* 表示器、GPU の加工器、アセットモジュール、更新器、シーンを所有し、フレームの
	* 順に呼ぶ。描画が読むフレームごとの状態（アニメーション、
	* コンストレイント、マテリアル、その他のアセット）を進め、フレームの間に
	* エディター・ゲーム・キャンバスのビューを集めて、フレームの終わりに
	* SceneRenderer でまとめて描画する。
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
		Graphics();

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
		~Graphics();

		/**
		* [EN]
		* Initializes the render device, the presenter (swap chain and screen
		* overlays) for window and its current client size, the GPU processor,
		* the asset module and the scene renderer, and prepares DLSS, which
		* EngineCore has started beforehand, on the device. The splash
		* system's boot config supplies the boot screen's images, which are
		* uploaded once here. gameConfig then sets the output resolution,
		* upscale mode, Reflex, DeepDVC and frame generation. Returns false if
		* any required service fails to initialize.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 描画デバイス、window と現在のクライアントサイズに対する表示器
		* （スワップチェインと画面オーバーレイ）、GPU の加工器、アセット
		* モジュール、SceneRenderer を初期化し、EngineCore が先に起動した
		* DLSS をデバイスに対して準備する。スプラッシュシステムのブート設定が
		* ブート画面の画像を与え、ここで一度だけアップロードする。その後
		* gameConfig から出力解像度、アップスケールモード、Reflex、DeepDVC、
		* フレーム生成を設定する。必要なサービスのいずれかの初期化に失敗した
		* 場合は false を返す。
		*/
		Bool Initialize(const Window& window, const SplashSystem& splashSystem, const GameConfig& gameConfig);

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
		* Applies the presentation and editor settings. Safe to call every
		* frame: the output and render sizes are only recorded (the render
		* graph rebuilds its resources from the view sizes), and the swap chain
		* is recreated only when frame generation actually changes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 表示とエディターの設定を反映する。毎フレーム呼んでよい。出力と描画の
		* サイズは記録するだけで（レンダーグラフはビューのサイズから自分の
		* リソースを作り直す）、スワップチェインはフレーム生成が実際に変わった
		* ときだけ作り直す。
		*/
		void Configure(const GraphicsSettings& settings);

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
		* Renders the views collected so far, once per frame, then binds the
		* current swap chain back buffer as the active render target, so the
		* editor UI drawn after it shows this frame's views.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ここまでに集めたビューをフレームに 1 回描画してから、現在の
		* スワップチェインバックバッファをアクティブなレンダーターゲットとして
		* バインドする。この後に描くエディター UI が、このフレームのビューを
		* 映すようにするため。
		*/
		void Bind();

		/**
		* [EN]
		* Renders the collected views if Bind has not already done so, restores
		* the back buffer to present state, submits and presents the frame, and
		* advances deferred descriptor retirement on every heap of the device.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Bind がまだ描画していなければ集めたビューを描画し、バックバッファを
		* present 状態へ戻してフレームを送信・表示し、デバイスの全ヒープで
		* 遅延したディスクリプタの回収を進める。
		*/
		void End();

	public:
		/**
		* [EN]
		* Advances the per-frame state that rendering reads (fonts, movies,
		* effects, colliders and shapes, then constraints, animation,
		* constraints again and materials) by deltaTime. Called once per
		* frame, after Begin and before the views are added, since every view
		* of a frame reads the same state; the caller chooses whose time
		* (the editor's or the game's) advances it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 描画が読むフレームごとの状態（フォント、ムービー、エフェクト、
		* コライダー、形、続いてコンストレイント、アニメーション、もう一度
		* コンストレイント、マテリアル）を deltaTime だけ進める。フレームの
		* 全ビューが同じ状態を読むので、Begin の後、ビューを足す前にフレームに
		* 1 回呼ぶ。どちらの時間（エディターかゲームか）で進めるかは、
		* 呼び出し側が選ぶ。
		*/
		void Update(Float deltaTime, World& world);

	public:
		/**
		* [EN]
		* Adds the editor view, seen from editorCamera, to this frame's views.
		* The selection (the shape debug display and the outline) is read from
		* the Selected tag in the World, not passed in.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* editorCamera から見たエディタービューを、このフレームのビューに足す。
		* 選択（形のデバッグ表示とアウトライン）は、引数で受け取らず、World の
		* Selected タグから読む。
		*/
		void EditorRender(WorldTimer& timer, const EditorCamera& editorCamera, World& world, RenderViewMode viewMode);

		/**
		* [EN]
		* Resolves the active game camera through cameraSystem and adds the
		* game view to this frame's views. The view is shown on output at
		* outputRect (x, y, width, height in the output's pixels): the editor
		* passes its own texture and the whole of it, the runtime passes the
		* screen and the letterbox rectangle. The camera is resolved every
		* frame whether or not the view is drawn.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* cameraSystem でアクティブなゲームカメラを解決し、ゲームビューを
		* このフレームのビューに足す。ビューは output の outputRect（出力先の
		* ピクセルでの x、y、幅、高さ）に出す。エディターは自分のテクスチャと
		* その全体を、ランタイムは画面とレターボックスの矩形を渡す。カメラは、
		* ビューを描くかどうかに関わらず毎フレーム解決する。
		*/
		void GameRender(GameTimer& timer, CameraSystem& cameraSystem, World& world, RenderViewOutput output, const Vector4& outputRect);

		/**
		* [EN]
		* Adds the canvas view, seen from its dedicated screen-space camera,
		* to this frame's views.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 専用のスクリーンスペースカメラから見たキャンバスビューを、この
		* フレームのビューに足す。
		*/
		void CanvasRender(WorldTimer& timer, const CanvasCamera& canvasCamera, World& world);

	public:
		/**
		* [EN]
		* Draws and presents the splash or boot screen for the current splash
		* phase. It runs a whole presentation frame by itself (Begin, draw,
		* End), so it is called in place of the normal frame, never between
		* Begin and End.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在のスプラッシュフェーズに対応するスプラッシュ画面またはブート画面を
		* 描画して表示する。表示フレーム全体（Begin、描画、End）を単独で行う
		* ため、通常フレームの代わりに呼び出し、Begin と End の間では呼び出さない。
		*/
		void DrawSplashScreen(const SplashSystem& splashSystem);

	public:
		/**
		* [EN]
		* Returns the render device: the D3D12 context, shader cache, descriptor
		* heaps and pipeline caches shared by every graphics service. Like every
		* service getter below, the reference stays valid until Finalize.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 描画デバイスを返す。全グラフィックスサービスで共有する D3D12
		* コンテキスト、シェーダーキャッシュ、ディスクリプタヒープ、
		* パイプラインのキャッシュを持つ。以下のサービス取得関数と同様に、
		* 参照は Finalize まで有効である。
		*/
		NODISCARD RenderDevice& Device()const;

		/**
		* [EN]
		* Returns GraphicsEngine's asset module: its loaders and resources.
		* The application registers its resources with ResourceCache through
		* GraphicsModule::Install, and GraphicsEngine code reaches the
		* resources by type through GraphicsModule::Resources.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* GraphicsEngine のアセットモジュール（ローダーとリソース）を返す。
		* アプリケーションは GraphicsModule::Install でそのリソースを
		* ResourceCache へ登録し、GraphicsEngine のコードは
		* GraphicsModule::Resources で型付きでリソースへ届く。
		*/
		NODISCARD GraphicsModule& Module()const;

		/**
		* [EN]
		* Returns the per-pass GPU timing of the scene renderer's graph.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* SceneRenderer のグラフの、パスごとの GPU 計測を返す。
		*/
		NODISCARD const GpuProfiler& Profiler()const;

	public:
		/**
		* [EN]
		* Returns the handles of the textures the editor UI shows for the
		* editor, game and canvas views, rewritten by Graphics after rendering
		* every frame.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エディター UI がエディター・ゲーム・キャンバスのビューに表示する
		* テクスチャのハンドルを返す。Graphics が毎フレーム描画の後に書き直す。
		*/
		NODISCARD const GraphicsDisplay& Display()const;

	private:
		/**
		* [EN]
		* Every member of Graphics. It is defined in Graphics.cpp, so the
		* render graph, the scene renderer and every Feature header stay out of
		* the code that includes Graphics.h (the editor and the runtime).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Graphics の全メンバー。Graphics.cpp で定義するので、レンダーグラフ、
		* SceneRenderer、全 Feature のヘッダーは、Graphics.h をインクルード
		* するコード（エディターとランタイム）から見えない。
		*/
		struct Impl;

		/// [EN] Owns every member of Graphics; created by the constructor.
		/// [JP] Graphics の全メンバーを所有する。コンストラクターで作る。
		ResourcePtr<Impl> impl_;
	};
}
