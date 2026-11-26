#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class Crister;
	class ResourceCache;

	class SEEDCORE_API ModelResource :public AssetResource<Crister>
	{
	public:
		ModelResource(GraphicsLoaderSystem& loaders);
		~ModelResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<Crister> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		void Unload(Uint32 assetID)override;

		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		Crister* Resolve(const Handle<Crister>& handle);

	private:
		GraphicsLoaderSystem& loaders_;
	};
}
