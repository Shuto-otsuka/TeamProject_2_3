#pragma once
#include <GraphicsEngine/Screen/Boot/BootTexture.h>
#include <GraphicsEngine/Screen/Splash/SplashTexture.h>

namespace SeedCore
{
	class RenderDevice;
	class SceneRenderer;
	class SplashSystem;

	/**
	* [EN]
	* Draws the screens shown before the game starts (the splash images and
	* the boot loading screen) straight into the back buffer. It owns a small
	* SceneRenderer holding only the Screen feature set, the fixed splash
	* images, and the boot screen's images read from the boot config.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームが始まる前の画面（スプラッシュの絵とブートのローディング画面）を、
	* バックバッファへ直接描く。Screen の組の Feature だけを持つ小さな SceneRenderer、
	* 固定のスプラッシュの絵、ブートの設定から読んだブート画面の絵を持つ。
	*/
	class GraphicsScreen
	{
	public:
		/**
		* [EN]
		* Keeps the device. Nothing touches the GPU until Initialize.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* デバイスを受け取る。Initialize まで GPU には触らない。
		*/
		explicit GraphicsScreen(RenderDevice& device);

		~GraphicsScreen();

		/**
		* [EN]
		* Creates the renderer, loads the splash images and the boot screen's images from config, and takes the back buffer's render target view slot.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レンダラーを作り、スプラッシュの絵と、config からブート画面の絵を読み込み、バックバッファの RTV の枠を確保する。
		*/
		void Initialize(const BootConfig& config);

		/**
		* [EN]
		* Releases the images, the render target view slot and the renderer; the device must be idle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 絵、RTV の枠、レンダラーを解放する。デバイスは待機済みであること。
		*/
		void Finalize();

	public:
		/**
		* [EN]
		* Draws splashSystem's current phase into screen: the boot screen during the boot phase, the splash image otherwise.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* splashSystem の今のフェーズを screen へ描く。ブートのフェーズならブート画面、それ以外はスプラッシュの絵。
		*/
		void Render(ID3D12Resource* screen, const SplashSystem& splashSystem);

	private:
		/// [EN] Device the renderer and the images are made on.
		/// [JP] レンダラーと絵を作るデバイス。
		RenderDevice& device_;

		/// [EN] Renderer holding only the Screen feature set.
		/// [JP] Screen の組の Feature だけを持つレンダラー。
		ResourcePtr<SceneRenderer> renderer_;

		/// [EN] The fixed splash images.
		/// [JP] 固定のスプラッシュの絵。
		SplashTexture splash_;

		/// [EN] The boot screen's background, bar and frame images.
		/// [JP] ブート画面の背景・バー・枠の絵。
		BootTexture boot_;

		/// [EN] Render target view slot the back buffer is written into every frame.
		/// [JP] バックバッファを毎フレーム書き込む RTV の枠。
		Uint32 screenView_ = SC_INVALID;
	};
}
