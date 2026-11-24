#pragma once
#include <GraphicsEngine/Effect/EffectLoader.h>
#include <FoundationEngine/Resource/Asset/AssetResource.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;

	class SEEDCORE_API EffectResource :public AssetResource<EffectAsset>
	{
	public:
		EffectResource(GraphicsLoaderSystem& loaders);
		~EffectResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<EffectAsset> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		void Unload(Uint32 assetID)override;

		Bool Bake(const AssetContext& context, Uint32 assetID, String filePath)override;

		EffectAsset* Resolve(const Handle<EffectAsset>& handle);

	private:
		GraphicsLoaderSystem& loaders_;
	};
}