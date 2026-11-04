#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* Startup configuration handed to the engine before the main window
	* and graphics device are created.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* メインウィンドウとグラフィックスデバイスが作成される前に、
	* エンジンへ渡される起動時設定。
	*/
	struct SEEDCORE_API Bootstrap
	{
		/**
		* [EN]
		* Switches the C runtime's character encoding to UTF-8, so narrow
		* strings used as paths are read as UTF-8 and paths containing
		* non-ASCII characters (e.g. Japanese folder names) resolve correctly.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* C ランタイムの文字コードを UTF-8 に切り替える。これにより、パスと
		* して使う char 文字列が UTF-8 として解釈され、日本語のフォルダ名など
		* 非 ASCII 文字を含むパスも正しく扱える。
		*/
		Bootstrap();

		/**
		* [EN]
		* Initial main window parameters.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* メインウィンドウの初期パラメータ。
		*/
		struct Window
		{
			/// [EN] Initial client width, in pixels.
			/// [JP] 初期クライアント幅（ピクセル）。
			Uint32 Width_ = 1920;

			/// [EN] Initial client height, in pixels.
			/// [JP] 初期クライアント高さ（ピクセル）。
			Uint32 Height_ = 1080;

			/// [EN] Whether to start in fullscreen mode.
			/// [JP] フルスクリーンモードで開始するかどうか。
			Bool Fullscreen_ = false;

			/// [EN] Window title text.
			/// [JP] ウィンドウタイトルの文字列。
			const Wchar* Title_ = L"SeedCore Engine";
		};

		/// [EN] Initial main window configuration.
		/// [JP] メインウィンドウの初期設定。
		Window WindowDesc_;
	};
}