#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Font/Font.h>

namespace SeedCore
{
	/**
	* [EN]
	* Loads ".ttf"/".otf"/".ttc" font files into pooled Font objects: opens
	* the FreeType face, the HarfBuzz font and the msdfgen handle, and sets
	* up the dynamic MTSDF atlas. The pool owns every Font, handles identify
	* them, and Clear tears down every third-party handle before freeing
	* the slot. A font keeps no cache file: its atlas grows with the glyphs
	* the text actually asks for.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ".ttf"/".otf"/".ttc" フォントファイルを、プールされた Font へ読み込む。
	* FreeType の face、HarfBuzz のフォント、msdfgen のハンドルを開き、動的
	* MTSDF アトラスを用意する。全ての Font をプールが所有し、ハンドルで
	* 識別し、Clear がスロット解放前に各サードパーティのハンドルを破棄する。
	* フォントはキャッシュファイルを持たない（アトラスは、テキストが実際に
	* 求めたグリフに合わせて育つため）。
	*/
	class FontLoader :public AssetLoader<Font>
	{
	public:
		FontLoader() = default;
		~FontLoader() = default;

		/**
		* [EN]
		* Opens the font file at filePath and returns the handle of the new
		* Font, or a null handle when any of the third-party handles cannot
		* be opened. Glyphs are rasterised at one fixed size into the
		* atlas; the size text is drawn at is applied by the Text component,
		* since an MTSDF atlas scales without losing sharpness. thread is
		* unused: the atlas is filled later, on the main thread.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* filePath のフォントファイルを開き、新しい Font のハンドルを返す。
		* サードパーティのハンドルのどれかを開けなければ null のハンドルを
		* 返す。グリフは1つの固定サイズでアトラスへ描く。描画する文字の
		* 大きさは Text コンポーネントが決める（MTSDF のアトラスは鮮明さを
		* 失わずに拡大縮小できるため）。thread は使わない（アトラスは後で
		* メインスレッドで埋めるため）。
		*/
		Handle<Font> Load(String filePath, LoadThread thread);

		/**
		* [EN]
		* Tears down every third-party handle of the Font handle names, then
		* frees its slot.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* handle が示す Font の全サードパーティのハンドルを破棄してから、
		* スロットを解放する。
		*/
		void Clear(Handle<Font>& handle)noexcept;
	};
}
