#include <GraphicsEngine/Texture/TextureLoader.h>
#include <GraphicsEngine/Texture/TextureCreator.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>
#include <FoundationEngine/File/FileUtility.h>

namespace SeedCore
{
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
	TextureLoader::TextureLoader(D3D12Context& context, BindlessHeap& heap) :context_(context), heap_(heap)
	{
		/// No Code
	}

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
	Handle<Texture> TextureLoader::Load(String filePath, LoadThread thread)
	{
		Handle<Texture> handle = pool_.Create();
		Texture* texture = pool_.Get(handle);
		if (!texture)
		{
			return Handle<Texture>::null();
		}

		/// [EN] The file path is kept on the texture, since streaming recreates an evicted texture from it.
		/// [JP] ファイルの位置はテクスチャに残す。追い出されたテクスチャを、ストリーミングがそこから作り直すため。
		texture->handle_ = handle;
		texture->textureIndex_ = heap_.AllocateIndex();
		texture->filePath_ = filePath;

		TextureCreator::CreateTexturePath(context_.GetDevice(), context_.GetDirectQueue(), heap_.Heap(), filePath, texture->resource_, texture->textureIndex_);

		/// [EN] The allocation size is what streaming counts against its budget.
		/// [JP] 確保した大きさは、ストリーミングが予算と比べるのに使う。
		if (texture->resource_)
		{
			D3D12_RESOURCE_DESC desc = texture->resource_->GetDesc();
			texture->sizeBytes_ = context_.GetDevice()->GetResourceAllocationInfo(0, 1, &desc).SizeInBytes;
		}

		return handle;
	}

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
	Handle<Texture> TextureLoader::Adopt(Texture&& texture)
	{
		Handle<Texture> handle = AssetLoader<Texture>::Adopt(std::move(texture));
		Texture* adopted = pool_.Get(handle);
		if (adopted)
		{
			adopted->handle_ = handle;
		}
		return handle;
	}

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
	void TextureLoader::Clear(Handle<Texture>& handle)noexcept
	{
		Texture* texture = pool_.Get(handle);

		/// [EN] An evicted texture already gave its resource and index back, so only a resident one returns them here.
		/// [JP] 追い出されたテクスチャはリソースと番号を既に返しているため、ここで返すのは常駐しているものだけ。
		if (texture && texture->resource_)
		{
			heap_.Release(std::move(texture->resource_), texture->textureIndex_);
		}
		pool_.Destroy(handle);
	}

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
	Bool TextureLoader::Bake(String sourcePath, String cachePath)
	{
		DynamicArray<Uint8> sourceBytes = FileUtility::LoadFileBinary(sourcePath);
		if (sourceBytes.empty())
		{
			return false;
		}

		/// [EN] The cache keeps the source bytes as they are, so it decodes exactly like the source; what it adds is encryption.
		/// [JP] キャッシュはソースのバイト列をそのまま持つため、ソースと全く同じにデコードされる。加わるのは暗号化だけ。
		BinaryOutputArchive cacheArchive;
		cacheArchive.Field("data", sourceBytes);
		return cacheArchive.Write(cachePath);
	}
}
