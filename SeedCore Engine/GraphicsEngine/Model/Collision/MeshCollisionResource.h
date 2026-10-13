#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <GraphicsEngine/Model/Collision/MeshCollision.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;

	class SEEDCORE_API MeshCollisionResource :public Asset, public NonCopyable
	{
	public:
		MeshCollisionResource(GraphicsLoaderSystem& loaders);
		~MeshCollisionResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		void Unload(Uint32 assetID)override;

		Handle<MeshCollision> Load(ResourceCache& cache, Uint32 assetID);

		Handle<MeshCollision> GetHandle(Uint32 assetID)const;

		MeshCollision* Resolve(const Handle<MeshCollision>& handle);

		Bool Contains(Uint32 assetID)const;

	private:
		GraphicsLoaderSystem& loaders_;

		FlatMap<Uint32, Handle<MeshCollision>> assetHandleMap_;
	};
}
