#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerLoader.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;

	class EffekseerResource :public AssetResource<EffekseerEffect>
	{
	public:
		EffekseerResource(GraphicsLoaderSystem& loaders);
		~EffekseerResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<EffekseerEffect> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		void Unload(Uint32 assetID)override;

		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		EffekseerEffect* Resolve(const Handle<EffekseerEffect>& handle);

	private:
		GraphicsLoaderSystem& loaders_;
	};
}
