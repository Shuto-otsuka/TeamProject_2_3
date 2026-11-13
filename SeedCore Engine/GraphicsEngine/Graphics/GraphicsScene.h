#pragma once
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <GraphicsEngine/Graphics/GraphicsDisplay.h>
#include <GraphicsEngine/RenderGraph/RenderGraph.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>
#include <GraphicsEngine/Quality/Upscale.h>

namespace SeedCore
{
	struct CelestialResult;
	struct ShapeList;
	struct GraphicsSettings;

	class CameraSystem;
	class CanvasCamera;
	class EditorCamera;
	class GameTimer;
	class GpuProfiler;
	class GraphicsModule;
	class RenderDevice;
	class SceneRenderer;
	class World;
	class WorldTimer;

	/**
	* [EN]
	* Collects the editor, game and canvas views during the frame and renders
	* them all at once through SceneRenderer. It owns the scene renderer,
	* the render and output sizes the views are built at, and the handles of
	* the views' output textures the editor UI shows.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームの間にエディター・ゲーム・キャンバスのビューを集め、
	* SceneRenderer でまとめて描画する。SceneRenderer と、ビューを組み立てる
	* 描画と出力のサイズと、エディター UI が表示する各ビューの出力テクスチャの
	* ハンドルを持つ。
	*/
	class GraphicsScene
	{
	public:
		/**
		* [EN]
		* Keeps the device the scene renderer shares, and the asset module
		* whose resources the views are rendered with. Nothing touches the
		* GPU until Initialize.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* SceneRenderer が共有するデバイスと、ビューの描画に使うリソースを持つ
		* アセットモジュールを受け取る。Initialize まで GPU には触らない。
		*/
		GraphicsScene(RenderDevice& device, GraphicsModule& module);

		/**
		* [EN]
		* Destroys the scene. The scene renderer is released by Finalize beforehand.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シーンを破棄する。SceneRenderer は、その前に Finalize が解放する。
		*/
		~GraphicsScene();

		/**
		* [EN]
		* Creates the scene renderer on the device, and takes the render
		* target view slot the screen is imported through.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デバイスに SceneRenderer を作り、画面を取り込むための RTV の枠を取る。
		*/
		void Initialize();

		/**
		* [EN]
		* Releases the views' output textures, the screen's render target
		* view slot and the scene renderer. Called after the GPU has finished.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 各ビューの出力テクスチャ、画面の RTV の枠、SceneRenderer を解放する。
		* GPU が処理を終えた後に呼ぶ。
		*/
		void Finalize();

	public:
		/**
		* [EN]
		* Records the output size, and the render size the upscale mode's
		* render scale gives for it. The render graph rebuilds its resources
		* from the view sizes, so nothing is recreated here.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 出力のサイズと、アップスケールのモードの倍率から決まる描画のサイズを
		* 記録する。レンダーグラフはビューのサイズから自分のリソースを作り直す
		* ので、ここでは何も作り直さない。
		*/
		void Configure(const GraphicsSettings& settings);

	public:
		/**
		* [EN]
		* Starts a frame: empties the collected views and allows one render again.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* フレームを始める。集めたビューを空にし、再び 1 回描画できるようにする。
		*/
		void Begin();

		/**
		* [EN]
		* Adds the editor view, seen from editorCamera, to this frame's views.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* editorCamera から見たエディタービューを、このフレームのビューに足す。
		*/
		void AddEditorView(WorldTimer& timer, const EditorCamera& editorCamera, World& world, RenderViewMode viewMode, RenderDebugSource debugSource, Uint64 showFlags);

		/**
		* [EN]
		* Resolves the active game camera through cameraSystem and adds the
		* game view to this frame's views, shown on output at outputRect
		* (x, y, width, height in the output's pixels). The camera is resolved
		* every frame whether or not the view is drawn.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* cameraSystem でアクティブなゲームカメラを解決し、ゲームビューを
		* このフレームのビューに足す。ビューは output の outputRect（出力先の
		* ピクセルでの x、y、幅、高さ）に出す。カメラは、ビューを描くかどうかに
		* 関わらず毎フレーム解決する。
		*/
		void AddGameView(GameTimer& timer, CameraSystem& cameraSystem, World& world, RenderViewOutput output, const Vector4& outputRect);

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
		void AddCanvasView(WorldTimer& timer, const CanvasCamera& canvasCamera, World& world);

		/**
		* [EN]
		* Renders every view collected this frame through SceneRenderer, once
		* per frame; later calls in the same frame do nothing. A view shown on
		* the screen is rendered into screen, the current back buffer in the
		* render-target state. celestial is this frame's sun, moon and night
		* factor.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このフレームに集めた全ビューを、SceneRenderer でフレームに 1 回
		* 描画する。同じフレームの 2 回目以降の呼び出しは何もしない。画面に
		* 出すビューは、screen（render-target 状態の現在のバックバッファ）へ
		* 描く。celestial は、このフレームの太陽・月・夜の度合い。shapes は、
		* このフレームに集めた形とコライダー。
		*/
		void Render(ID3D12Resource* screen, const CelestialResult& celestial, const ShapeList& shapes);

	public:
		/**
		* [EN]
		* Returns the aspect ratio of the render size.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 描画のサイズの縦横比を返す。
		*/
		NODISCARD Float AspectRatio()const;

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

		/**
		* [EN]
		* Returns the handles of the views' output textures.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 各ビューの出力テクスチャのハンドルを返す。
		*/
		NODISCARD const GraphicsDisplay& Display()const;

	private:
		/// [EN] Device the scene renderer shares.
		/// [JP] SceneRenderer が共有するデバイス。
		RenderDevice& device_;

		/// [EN] Asset module whose resources the views are rendered with.
		/// [JP] ビューの描画に使うリソースを持つアセットモジュール。
		GraphicsModule& module_;

		/// [EN] Width the views are rendered at, after the render scale of the upscale mode.
		/// [JP] アップスケールのモードの倍率を適用した後の、ビューを描く幅。
		Uint32 nativeWidth_ = static_cast<Uint32>(ScResolution::SC_HD.Width);

		/// [EN] Height the views are rendered at, after the render scale of the upscale mode.
		/// [JP] アップスケールのモードの倍率を適用した後の、ビューを描く高さ。
		Uint32 nativeHeight_ = static_cast<Uint32>(ScResolution::SC_HD.Height);

		/// [EN] Output width the editor and game views are displayed at.
		/// [JP] エディターとゲームのビューを表示する出力の幅。
		Uint32 outputWidth_ = static_cast<Uint32>(ScResolution::SC_HD.Width);

		/// [EN] Output height the editor and game views are displayed at.
		/// [JP] エディターとゲームのビューを表示する出力の高さ。
		Uint32 outputHeight_ = static_cast<Uint32>(ScResolution::SC_HD.Height);

		/// [EN] Upscale method and mode, handed to the render each frame for the Upscale Feature to read.
		/// [JP] アップスケールの方式とモード。毎フレーム描画に渡し、Upscale の Feature が読む。
		GraphicsUpscale upscale_;

		/// [EN] True once the collected views have been rendered for the current frame.
		/// [JP] 現在のフレームで、集めたビューを描画し終えたら true。
		Bool rendered_ = false;

		/// [EN] Renders every collected view once per frame through the render graph.
		/// [JP] 集めた全ビューを、フレームに 1 回、レンダーグラフで描画する。
		ResourcePtr<SceneRenderer> renderer_;

		/// [EN] Handles of the views' output textures that the editor UI shows.
		/// [JP] エディター UI が表示する、各ビューの出力テクスチャのハンドル。
		GraphicsDisplay display_;

		/// [EN] Output texture of each view, imported into the render graph as ViewOutput and shown by the editor UI. Indexed by RenderViewKind.
		/// [JP] 各ビューの出力テクスチャ。ViewOutput としてレンダーグラフに取り込み、エディター UI が表示する。RenderViewKind の番号で引く。
		StaticArray<RenderGraphAllocation, RenderViewCount> outputs_;

		/// [EN] Slot in the device's render target heap that Render rewrites with the current back buffer's view, for the views shown on the screen.
		/// [JP] デバイスの RTV ヒープの枠。画面に出すビューのために、Render が現在のバックバッファのビューで書き直す。
		Uint32 screenView_ = SC_INVALID;

		/// [EN] Views collected by AddEditorView, AddGameView and AddCanvasView during the frame.
		/// [JP] フレームの間に AddEditorView、AddGameView、AddCanvasView が集めたビュー。
		StaticArray<RenderView, RenderViewCount> views_;

		/// [EN] Number of entries of views_ in use this frame.
		/// [JP] このフレームで使っている views_ の数。
		Uint32 viewCount_ = 0;

		/// [EN] World the collected views show; every view of a frame shows the same World.
		/// [JP] 集めたビューが映す World。1 フレームのビューは、どれも同じ World を映す。
		World* world_ = nullptr;
	};
}
