#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Movie/Video.h>

namespace SeedCore
{
	/**
	* [EN]
	* Loads ".mp4"/".movie" assets into pooled Video objects: starts up
	* Media Foundation once, opens the decoder per asset and reads the
	* native frame size. The pool owns every Video, handles identify them,
	* and Clear tears the decoder and its GPU resources down before freeing
	* the slot. Writing the ".movie" cache is private and reached only by
	* MovieResource.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ".mp4"/".movie" アセットを、プールされた Video へ読み込む。Media
	* Foundation を一度だけ起動し、アセットごとにデコーダを開いて元の
	* フレームサイズを読む。全ての Video をプールが所有し、ハンドルで識別し、
	* Clear がスロット解放前にデコーダと GPU リソースを破棄する。".movie"
	* キャッシュの書き出しは private で、MovieResource からだけ使う。
	*/
	class MovieLoader :public AssetLoader<Video>
	{
	private:
		friend class MovieResource;

	public:
		MovieLoader();

		~MovieLoader();

		/**
		* [EN]
		* Opens the video at filePath (a source, or a ".movie" cache) and
		* returns the handle of the new Video, or a null handle when Media
		* Foundation is unavailable or the video cannot be opened. A source
		* is read from its sibling ".movie" cache when that is newer. thread
		* is unused: the decoder keeps no per-thread state.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* filePath の動画（ソースか ".movie" キャッシュ）を開き、新しい Video の
		* ハンドルを返す。Media Foundation が使えない、または動画を開けない
		* 場合は null のハンドルを返す。ソースは、隣の ".movie" キャッシュの
		* 方が新しければそちらから読む。thread は使わない（デコーダはスレッド
		* ごとの状態を持たないため）。
		*/
		Handle<Video> Load(String filePath, LoadThread thread);

		/**
		* [EN]
		* Tears the decoder and the GPU resources of the Video handle names
		* down, then frees its slot.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* handle が示す Video のデコーダと GPU リソースを破棄してから、
		* スロットを解放する。
		*/
		void Clear(Handle<Video>& handle)noexcept;

	private:
		/**
		* [EN]
		* Writes the bytes of the source video at sourcePath, AES-256-CBC
		* encrypted behind a random 16-byte IV, to the ".movie" cache at
		* cachePath. The cache has no field framing, so the decoder can
		* decrypt any requested range without parsing the whole file.
		* Returns false when the source cannot be read or the write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* sourcePath のソース動画のバイト列を、ランダムな 16 バイトの IV の後に
		* AES-256-CBC で暗号化して、cachePath の ".movie" キャッシュへ書く。
		* キャッシュはフィールド形式を持たないため、デコーダはファイル全体を
		* 解析せずに任意の範囲を復号できる。ソースを読めない、または書き込みに
		* 失敗した場合は false を返す。
		*/
		Bool Bake(String sourcePath, String cachePath);

	private:
		Bool ownsComInitialize_ = false;

		Bool mfStarted_ = false;
	};
}
