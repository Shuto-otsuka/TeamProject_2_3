#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class Skymap;
	class ResourceCache;

	class SkymapResource :public AssetResource<Skymap>
	{
	public:
		SkymapResource(GraphicsLoaderSystem& loaders);
		~SkymapResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<Skymap> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		void Unload(Uint32 assetID)override;

		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		Skymap* Resolve(const Handle<Skymap>& handle);

	private:
		GraphicsLoaderSystem& loaders_;
	};
}
