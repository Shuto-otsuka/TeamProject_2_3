#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>
#include <GraphicsEngine/Texture/Texture.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;

	class SEEDCORE_API TextureResource :public AssetResource<Texture>
	{
	private:
		friend class TextureStreaming;

	public:
		TextureResource(GraphicsLoaderSystem& loaders);
		~TextureResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<Texture> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		void Unload(Uint32 assetID)override;

		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		Texture* Resolve(const Handle<Texture>& handle);

	private:
		GraphicsLoaderSystem& loaders_;
	};
}
