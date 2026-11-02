#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <GraphicsEngine/Sky/Skymap.h>

namespace SeedCore
{
	class BindlessHeap;
	class D3D12Context;

	/**
	* [EN]
	* Loads ".hdr" equirectangular sources and prebuilt ".skymap" caches into
	* pooled Skymap objects. The pool owns every Skymap, handles identify
	* them, and Clear releases both the GPU texture and the slot. Writing
	* the ".skymap" cache is private and reached only by SkymapResource.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ".hdr" の equirect ソースと、ビルド済みの ".skymap" キャッシュを、
	* プールされた Skymap へ読み込む。全ての Skymap をプールが所有し、
	* ハンドルで識別し、Clear が GPU テクスチャとスロットの両方を解放する。
	* ".skymap" キャッシュの書き出しは private で、SkymapResource からだけ
	* 使う。
	*/
	class SkymapLoader :public AssetLoader<Skymap>
	{
	private:
		friend class SkymapResource;

	public:
		/**
		* [EN]
		* Takes the device context skymaps are uploaded with and the
		* bindless heap their textures are registered in.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スカイマップをアップロードするデバイスのコンテキストと、テクスチャの
		* 登録先のバインドレスヒープを受け取る。
		*/
		SkymapLoader(D3D12Context& context, BindlessHeap& heap);
		~SkymapLoader() = default;

		/**
		* [EN]
		* Reads the ".hdr" source or ".skymap" cache at filePath, uploads it
		* through the direct queue and returns the handle of the new Skymap,
		* or a null handle when it cannot be read or uploaded. Nothing is
		* written. thread is unused: the upload waits on the direct queue.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* filePath の ".hdr" ソースか ".skymap" キャッシュを読み、Direct キュー
		* でアップロードして、新しい Skymap のハンドルを返す。読めない、または
		* アップロードできない場合は null のハンドルを返す。何も書き出さない。
		* thread は使わない（アップロードは Direct キューで待つため）。
		*/
		Handle<Skymap> Load(String filePath, LoadThread thread);

		/**
		* [EN]
		* Hands the Skymap's texture to the heap's deferred release, then
		* frees its slot.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Skymap のテクスチャをヒープの遅延解放へ渡してから、スロットを
		* 解放する。
		*/
		void Clear(Handle<Skymap>& handle)noexcept;

	private:
		/**
		* [EN]
		* Decodes the ".hdr" source at sourcePath and writes its pixels to the
		* ".skymap" cache at cachePath, so the skymap can be shipped and
		* loaded without the source. Returns false when the source cannot be
		* decoded or the write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* sourcePath の ".hdr" ソースをデコードし、そのピクセルを cachePath の
		* ".skymap" キャッシュへ書く。ソース無しでスカイマップを配布・読み込み
		* できるようにするため。ソースをデコードできない、または書き込みに
		* 失敗した場合は false を返す。
		*/
		Bool Bake(String sourcePath, String cachePath);

	private:
		/// [EN] Device context skymaps are uploaded with.
		/// [JP] スカイマップをアップロードするデバイスのコンテキスト。
		D3D12Context& context_;

		/// [EN] Bindless heap the skymaps' textures are registered in.
		/// [JP] スカイマップのテクスチャが登録されるバインドレスヒープ。
		BindlessHeap& heap_;
	};
}
