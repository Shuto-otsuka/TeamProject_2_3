#pragma once
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerLoader.h>

namespace SeedCore
{
	struct GraphicsLoaderSystem;

	class ResourceCache;

	class EffekseerResource :public Asset, public NonCopyable
	{
	public:
		EffekseerResource(GraphicsLoaderSystem& loaders);
		~EffekseerResource() = default;

		void Load(const AssetContext& context, Uint32 assetID)override;

		void Unload(Uint32 assetID)override;

		Handle<EffekseerEffect> Load(ResourceCache& cache, Uint32 assetID);

		Handle<EffekseerEffect> GetHandle(Uint32 assetID)const;

		EffekseerEffect* Resolve(const Handle<EffekseerEffect>& handle);

		Bool Contains(Uint32 assetID)const;

	private:
		GraphicsLoaderSystem& loaders_;

		FlatMap<Uint32, Handle<EffekseerEffect>> assetHandleMap_;
	};
}
