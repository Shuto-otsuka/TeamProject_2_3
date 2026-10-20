#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <GraphicsEngine/Model/Animation/Animation.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;

	class SEEDCORE_API AnimationResource :public Asset, public NonCopyable
	{
	public:
		AnimationResource(GraphicsLoaderSystem& loaders);
		~AnimationResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		Handle<Animation> Load(ResourceCache& cache, Uint32 assetID);

		void Unload(Uint32 assetID)override;

		Handle<Animation> GetHandle(Uint32 assetID)const;

		Animation* Resolve(const Handle<Animation>& handle);

		Bool Contains(Uint32 assetID)const;

	private:
		GraphicsLoaderSystem& loaders_;

		FlatMap<Uint32, Handle<Animation>> assetHandleMap_;
	};
}
