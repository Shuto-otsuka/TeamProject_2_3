#pragma once
#include <FoundationEngine/Resource/Asset/AssetResource.h>
#include <GraphicsEngine/Model/Collision/MeshCollision.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;
	class ModelResource;

	class SEEDCORE_API MeshCollisionResource :public AssetResource<MeshCollision>
	{
	public:
		MeshCollisionResource(GraphicsLoaderSystem& loaders, ModelResource& modelResource);
		~MeshCollisionResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<MeshCollision> Load(ResourceCache& cache, Uint32 assetID, LoadThread thread = LoadThread::Main);

		void Unload(Uint32 assetID)override;

		Bool Generate(const AssetContext& context, Uint32 sourceAssetID, DynamicArray<String>& writtenPaths)override;

		MeshCollision* Resolve(const Handle<MeshCollision>& handle);

	private:
		GraphicsLoaderSystem& loaders_;

		ModelResource& modelResource_;
	};
}
