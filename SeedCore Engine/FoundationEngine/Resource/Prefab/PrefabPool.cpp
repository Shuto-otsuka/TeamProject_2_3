#include <FoundationEngine/Resource/Prefab/PrefabPool.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	Handle<PrefabBlueprint> PrefabPool::Load(Uint32 assetID, ResourceCache& cache)
	{
		auto it = loaded_.find(assetID);
		if (it != loaded_.end())
		{
			return it->second;
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<PrefabBlueprint>::null();
		}

		Handle<PrefabBlueprint> handle = pool_.Create();
		PrefabBlueprint* prefab = pool_.Get(handle);
		if (!prefab)
		{
			return Handle<PrefabBlueprint>::null();
		}

		try
		{
			if (!prefab->Read(asset->fullpath_.c_str()))
			{
				SC_LOG_ERROR("Prefabファイルを開けませんでした: {}", asset->fullpath_.str());
				return Handle<PrefabBlueprint>::null();
			}
		}
		catch (...)
		{
			SC_LOG_ERROR("Prefabファイルの読み込みに失敗しました: {}", asset->fullpath_.str());
			return Handle<PrefabBlueprint>::null();
		}

		loaded_[assetID] = handle;
		return handle;
	}

	PrefabBlueprint* PrefabPool::Get(Handle<PrefabBlueprint> handle)
	{
		return pool_.Get(handle);
	}
}
