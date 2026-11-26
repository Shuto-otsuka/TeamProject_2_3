#include <GraphicsEngine/Texture/TextureLoader.h>
#include <GraphicsEngine/Texture/TextureCreator.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <FoundationEngine/Serialization/Binary/BinaryArchive.h>
#include <FoundationEngine/File/FileUtility.h>

namespace SeedCore
{
	TextureLoader::TextureLoader(D3D12Context& context, BindlessHeap& heap) :context_(context), heap_(heap)
	{
		/// No Code
	}

	Handle<Texture> TextureLoader::Load(String filePath, LoadThread thread)
	{
		Handle<Texture> handle = pool_.Create();
		Texture* texture = pool_.Get(handle);
		if (!texture)
		{
			return Handle<Texture>::null();
		}

		texture->handle_ = handle;
		texture->textureIndex_ = heap_.AllocateIndex();
		texture->filePath_ = filePath;

		TextureCreator::CreateTexturePath(context_.GetDevice(), context_.GetCopyQueue(), context_.GetLoadTransferLane(thread), heap_.Heap(), filePath, texture->resource_, texture->textureIndex_);

		if (texture->resource_)
		{
			D3D12_RESOURCE_DESC desc = texture->resource_->GetDesc();
			texture->sizeBytes_ = context_.GetDevice()->GetResourceAllocationInfo(0, 1, &desc).SizeInBytes;
		}

		return handle;
	}

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

	void TextureLoader::Clear(Handle<Texture>& handle)noexcept
	{
		Texture* texture = pool_.Get(handle);

		if (texture && texture->resource_)
		{
			heap_.Release(std::move(texture->resource_), texture->textureIndex_);
		}
		pool_.Destroy(handle);
	}

	Bool TextureLoader::Bake(String sourcePath, String cachePath)
	{
		DynamicArray<Uint8> sourceBytes = FileUtility::LoadFileBinary(sourcePath);
		if (sourceBytes.empty())
		{
			return false;
		}

		BinaryOutputArchive cacheArchive;
		cacheArchive.Field("data", sourceBytes);
		return cacheArchive.Write(cachePath);
	}
}
