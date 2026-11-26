#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>
#include <GraphicsEngine/Model/Animation/Animation.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;

	class SEEDCORE_API AnimationResource :public AssetResource<Animation>
	{
	public:
		AnimationResource(GraphicsLoaderSystem& loaders);
		~AnimationResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<Animation> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		void Unload(Uint32 assetID)override;

		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		Bool Bake(const AssetContext& context, Uint32 assetID, String filePath)override;

		Animation* Resolve(const Handle<Animation>& handle);

	private:
		GraphicsLoaderSystem& loaders_;
	};
}
