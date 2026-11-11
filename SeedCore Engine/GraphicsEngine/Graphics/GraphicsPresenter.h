#pragma once
#include <GraphicsEngine/Shape/Screen/BootScreen.h>
#include <GraphicsEngine/Shape/Screen/FadeScreen.h>
#include <GraphicsEngine/Shape/Screen/LetterScreen.h>
#include <GraphicsEngine/Shape/Screen/SplashScreen.h>

namespace SeedCore
{
	struct GraphicsSettings;

	class RenderDevice;
	class SplashSystem;
	class SwapChain;
	class Window;

	/**
	* [EN]
	* Puts frames on the screen: owns the swap chain and the screen overlays
	* drawn straight onto the back buffer (splash, boot, fade and letterbox),
	* moves the back buffer between the present and render-target states,
	* and presents it. DLSS Reflex and Frame Generation, which act at
	* presentation, are reached through the Gateway.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームを画面に出す。スワップチェインと、バックバッファへ直接描く
	* 画面オーバーレイ（スプラッシュ、ブート、フェード、レターボックス）を
	* 持ち、バックバッファを present と render-target の状態の間で移して
	* 表示する。表示のときに働く DLSS の Reflex とフレーム生成には、
	* Gateway から届く。
	*/
	class GraphicsPresenter
	{
	public:
		/**
		* [EN]
		* Keeps the device the swap chain and overlays are created on.
		* Nothing touches the GPU until Initialize.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スワップチェインとオーバーレイを作るデバイスを受け取る。
		* Initialize まで GPU には触らない。
		*/
		explicit GraphicsPresenter(RenderDevice& device);

		/**
		* [EN]
		* Destroys the presenter. GPU resources are released by Finalize beforehand.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 表示器を破棄する。GPU リソースは、その前に Finalize が解放する。
		*/
		~GraphicsPresenter();

		/**
		* [EN]
		* Creates the swap chain for window at its current client size, and
		* the screen overlays. The splash system's boot config supplies the
		* boot screen's images, which are uploaded once here. Returns false
		* if the swap chain cannot be created.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* window の現在のクライアントサイズでスワップチェインを作り、画面
		* オーバーレイを作る。スプラッシュシステムのブート設定がブート画面の
		* 画像を与え、ここで一度だけアップロードする。スワップチェインを
		* 作れなければ false を返す。
		*/
		Bool Initialize(const Window& window, const SplashSystem& splashSystem);

		/**
		* [EN]
		* Releases the swap chain and the screen overlays. Called after the
		* GPU has finished and after DLSS has released its features.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スワップチェインと画面オーバーレイを解放する。GPU が処理を終え、
		* DLSS が機能を解放した後に呼ぶ。
		*/
		void Finalize();

	public:
		/**
		* [EN]
		* Applies vertical sync and frame generation. Safe to call every
		* frame: the swap chain is recreated only when frame generation
		* actually changes.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 垂直同期とフレーム生成を反映する。毎フレーム呼んでよい。
		* スワップチェインを作り直すのは、フレーム生成が実際に変わったときだけ。
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
		void Resize(Uint32 width, Uint32 height);

	public:
		/**
		* [EN]
		* Opens the presentation frame for DLSS, moves the current back buffer
		* from the present state to the render-target state, and clears it to
		* black.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* DLSS の表示フレームを開始し、現在のバックバッファを present 状態から
		* render-target 状態へ移して、黒で塗る。
		*/
		void Begin();

		/**
		* [EN]
		* Selects the current back buffer as the output-merger render target.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在のバックバッファを output-merger のレンダーターゲットとして選択する。
		*/
		void Bind();

		/**
		* [EN]
		* Moves the back buffer back to the present state. Called before the
		* frame's commands are submitted.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* バックバッファを present 状態へ戻す。フレームのコマンドを送信する
		* 前に呼ぶ。
		*/
		void End();

		/**
		* [EN]
		* Presents the back buffer. Called after the frame's commands are submitted.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* バックバッファを表示する。フレームのコマンドを送信した後に呼ぶ。
		*/
		void Present();

	public:
		/**
		* [EN]
		* Draws the boot screen or the splash screen of the current splash
		* phase onto the bound back buffer.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在のスプラッシュフェーズのブート画面かスプラッシュ画面を、
		* バインド済みのバックバッファへ描く。
		*/
		void DrawSplash(const SplashSystem& splashSystem);

	public:
		/**
		* [EN]
		* Returns the current back buffer. Between Begin and End it is in the
		* render-target state, cleared to black.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在のバックバッファを返す。Begin と End の間は render-target 状態で、
		* 黒で塗られている。
		*/
		NODISCARD ID3D12Resource* BackBuffer()const;

	private:
		/// [EN] Device the swap chain and overlays are created on and record into.
		/// [JP] スワップチェインとオーバーレイを作り、記録する先のデバイス。
		RenderDevice& device_;

		/// [EN] Presentation back buffers and swap chain state.
		/// [JP] 表示用バックバッファとスワップチェインの状態。
		ResourcePtr<SwapChain> swapChain_;

		/// [EN] Current swap chain width in pixels.
		/// [JP] 現在のスワップチェインの幅（ピクセル）。
		Uint32 width_ = 0;

		/// [EN] Current swap chain height in pixels.
		/// [JP] 現在のスワップチェインの高さ（ピクセル）。
		Uint32 height_ = 0;

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
