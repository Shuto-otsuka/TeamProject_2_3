#pragma once
#include <GraphicsEngine/Texture/Texture.h>
#include <FoundationEngine/Resource/Asset/AssetLoader.h>

namespace SeedCore
{
	class BindlessHeap;
	class D3D12Context;

	/**
	* [EN]
	* Loads image files into GPU textures owned by a handle pool. Writing
	* the ".texture" cache is private and reached only by TextureResource;
	* keeping textures under a memory budget is TextureStreaming's job.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 画像ファイルを、ハンドルのプールが所有する GPU テクスチャへ読み込む。
	* ".texture" キャッシュの書き出しは private で、TextureResource から
	* だけ使う。テクスチャをメモリ予算内に収めるのは TextureStreaming の
	* 役目。
	*/
	class SEEDCORE_API TextureLoader :public AssetLoader<Texture>
	{
	private:
		friend class TextureResource;

	public:
		/**
		* [EN]
		* Takes the device context and the bindless heap every texture is
		* created on and registered in.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全てのテクスチャを作るデバイスのコンテキストと、登録先の
		* バインドレスヒープを受け取る。
		*/
		TextureLoader(D3D12Context& context, BindlessHeap& heap);
		~TextureLoader() = default;

		/**
		* [EN]
		* Creates a texture from the image file at filePath and returns its
		* handle. The upload goes through the direct queue, so thread is not
		* needed to pick per-thread state.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* filePath の画像ファイルからテクスチャを作り、そのハンドルを返す。
		* アップロードは Direct キューを通すため、スレッドごとの状態を選ぶ
		* のに thread は要らない。
		*/
		Handle<Texture> Load(String filePath, LoadThread thread);

		/**
		* [EN]
		* Moves a texture built by TextureCreator into the pool and returns
		* its handle, recording the handle on the texture itself.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* TextureCreator が組み立てたテクスチャをプールへ移してハンドルを
		* 返し、テクスチャ自身にもそのハンドルを記録する。
		*/
		Handle<Texture> Adopt(Texture&& texture);

		/**
		* [EN]
		* Releases the texture handle names, returning its GPU resource and
		* shader resource view index to the heap.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* handle が示すテクスチャを解放し、GPU リソースとシェーダーリソース
		* ビューの番号をヒープへ返す。
		*/
		void Clear(Handle<Texture>& handle)noexcept;

	private:
		/**
		* [EN]
		* Writes the image bytes of sourcePath into the encrypted ".texture"
		* cache at cachePath. Returns false when the source cannot be read or
		* the write failed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* sourcePath の画像バイト列を、cachePath の暗号化された ".texture"
		* キャッシュへ書き出す。ソースを読めない、または書き込みに失敗した
		* 場合は false を返す。
		*/
		Bool Bake(String sourcePath, String cachePath);

	private:
		/// [EN] Device context the textures are created with.
		/// [JP] テクスチャを作るデバイスのコンテキスト。
		D3D12Context& context_;

		/// [EN] Bindless heap the textures' shader resource views live in.
		/// [JP] テクスチャのシェーダーリソースビューが置かれるバインドレスヒープ。
		BindlessHeap& heap_;
	};
}
