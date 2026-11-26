#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>
#include <GraphicsEngine/Model/Skeleton/Skeleton.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;
	class ModelResource;

	class SEEDCORE_API SkeletonResource :public AssetResource<SkeletonRig>
	{
	public:
		SkeletonResource(GraphicsLoaderSystem& loaders, ModelResource& modelResource);
		~SkeletonResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<SkeletonRig> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		void Unload(Uint32 assetID)override;

		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		Bool Bake(const AssetContext& context, Uint32 assetID, String filePath)override;

		SkeletonRig* Resolve(const Handle<SkeletonRig>& handle);

	private:
		GraphicsLoaderSystem& loaders_;

		ModelResource& modelResource_;
	};
}
