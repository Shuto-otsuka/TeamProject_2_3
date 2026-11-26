#include <FoundationEngine/Resource/Scene/ScenePool.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	Handle<SceneBlueprint> ScenePool::Load(Uint32 assetID, ResourceCache& cache)
	{
		auto it = loaded_.find(assetID);
		if (it != loaded_.end())
		{
			return it->second;
		}

		AssetRecord* asset = cache.Record(assetID);
		if (!asset)
		{
			return Handle<SceneBlueprint>::null();
		}

		Handle<SceneBlueprint> handle = pool_.Create();
		SceneBlueprint* scene = pool_.Get(handle);
		if (!scene)
		{
			return Handle<SceneBlueprint>::null();
		}

		try
		{
			if (!scene->Read(asset->fullpath_.c_str()))
			{
				SC_LOG_ERROR("Sceneファイルを開けませんでした: {}", asset->fullpath_.str());
				return Handle<SceneBlueprint>::null();
			}
		}
		catch (...)
		{
			SC_LOG_ERROR("Sceneファイルの読み込みに失敗しました: {}", asset->fullpath_.str());
			return Handle<SceneBlueprint>::null();
		}

		loaded_[assetID] = handle;
		return handle;
	}

	SceneBlueprint* ScenePool::Get(Handle<SceneBlueprint> handle)
	{
		return pool_.Get(handle);
	}

	SceneBlueprint& ScenePool::GetScratch()
	{
		return scratch_;
	}
}
