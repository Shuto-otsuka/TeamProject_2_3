#pragma once

namespace SeedCore
{
	struct GraphicsSettings;

	class RenderDevice;
	class SwapChain;
	class Window;

	/**
	* [EN]
	* Puts frames on the screen: owns the swap chain, moves the back buffer
	* between the present and render-target states, and presents it. DLSS
	* Reflex and Frame Generation, which act at presentation, are reached
	* through the Gateway.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームを画面に出す。スワップチェインを持ち、バックバッファを present と
	* render-target の状態の間で移して表示する。表示のときに働く DLSS の Reflex と
	* フレーム生成には、Gateway から届く。
	*/
	class GraphicsPresenter
	{
	public:
		/**
		* [EN]
		* Keeps the device the swap chain is created on.
		* Nothing touches the GPU until Initialize.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スワップチェインを作るデバイスを受け取る。
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
		* Creates the swap chain for window at its current client size.
		* Returns false if the swap chain cannot be created.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* window の現在のクライアントサイズでスワップチェインを作る。
		* スワップチェインを作れなければ false を返す。
		*/
		Bool Initialize(const Window& window);

		/**
		* [EN]
		* Releases the swap chain. Called after the
		* GPU has finished and after DLSS has released its features.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スワップチェインを解放する。GPU が処理を終え、
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
		* Opens the presentation frame for DLSS and moves the current back
		* buffer from the present state to the render-target state. Nothing
		* clears it: whatever is shown on the screen (HUD.Compose, the splash
		* and boot screens, ImGui) covers it all, letterbox bars included.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* DLSS の表示フレームを開始し、現在のバックバッファを present 状態から
		* render-target 状態へ移す。塗りつぶしはしない。画面に出すもの（HUD.Compose、
		* スプラッシュとブートの画面、ImGui）がレターボックスの帯も含めて全面を描く。
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
		* Returns the current back buffer. Between Begin and End it is in the
		* render-target state.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在のバックバッファを返す。Begin と End の間は render-target 状態。
		*/
		NODISCARD ID3D12Resource* BackBuffer()const;

	private:
		/// [EN] Device the swap chain is created on and records into.
		/// [JP] スワップチェインを作り、記録する先のデバイス。
		RenderDevice& device_;

		/// [EN] Presentation back buffers and swap chain state.
		/// [JP] 表示用バックバッファとスワップチェインの状態。
		ResourcePtr<SwapChain> swapChain_;
	};
}
