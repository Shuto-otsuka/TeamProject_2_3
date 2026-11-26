#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class Video;
	class ResourceCache;

	class MovieResource :public AssetResource<Video>
	{
	private:
		friend class MovieSystem;

	public:
		MovieResource(GraphicsLoaderSystem& loaders);
		~MovieResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<Video> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		void Unload(Uint32 assetID)override;

		void Clear()override;

		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		Video* Resolve(const Handle<Video>& handle);

	private:
		GraphicsLoaderSystem& loaders_;
	};
}
