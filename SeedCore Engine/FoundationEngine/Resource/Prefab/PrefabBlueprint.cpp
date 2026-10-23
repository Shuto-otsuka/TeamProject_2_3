#include <FoundationEngine/Resource/Prefab/PrefabBlueprint.h>
#include <FoundationEngine/Resource/Prefab/PrefabRegistry.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/Log/Error.h>
#include <FoundationEngine/Serialization/Json/JsonArchive.h>

namespace SeedCore
{
	/**
	* [EN]
	* Records root and every descendant into nodes_, replacing any
	* previously captured data.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* root とその全子孫を nodes_ へ記録し、以前に取得していたデータを
	* 置き換える。
	*/
	void PrefabBlueprint::Capture(Actor root)
	{
		nodes_.clear();
		BlueprintNode::Capture(root, nodes_);
	}

	/**
	* [EN]
	* Recreates the captured subtree as new actors in world, parented
	* under parent (if given). Returns the root of the newly created
	* subtree, or nullptr if nothing was instantiated. sourceAssetID is
	* stamped onto the new root so later "apply to prefab" edits know
	* which .prefab asset to overwrite.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 取得済みのサブツリーを、world 内に新しい actor 群として再生成する。
	* parent が指定されていればその下へ配置する。新しく生成された
	* サブツリーのルートを返す。何もインスタンス化されなければ nullptr
	* を返す。sourceAssetID は新しいルートへ刻印され、後の
	* 「プレハブに適用」編集がどの .prefab アセットを上書きすべきかを
	* 判断できるようにする。
	*/
	Actor PrefabBlueprint::Instantiate(World& world, ResourceCache& cache, Actor parent, Uint32 sourceAssetID)const
	{
		DynamicArray<Actor> instantiated = BlueprintNode::Instantiate(world, cache, nodes_, parent, true);
		if (instantiated.empty() || !instantiated[0])
		{
			return Actor();
		}

		/// [EN] Only the root is linked to the asset; the rest are already recorded as prefab-built by the instantiation.
		/// [JP] アセットに結ぶのはルートだけ。残りはインスタンス化の時点でプレハブ由来として記録済み。
		WorldAccessor::Services(world).Add<PrefabRegistry>()->Add(ActorAccessor::GetEntity(instantiated[0]).GetID(), sourceAssetID, true);
		return instantiated[0];
	}

	/**
	* [EN]
	* Writes this prefab's captured data to path as JSON. Returns
	* whether the file was written successfully.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この prefab の取得済みデータを、JSON として path へ書き込む。
	* ファイルが正常に書き込まれたかどうかを返す。
	*/
	Bool PrefabBlueprint::Write(const std::filesystem::path& path)
	{
		JsonOutputArchive archive;
		Save(archive);
		return archive.Write(String(path.string()));
	}

	/**
	* [EN]
	* Reads captured data from path (JSON), replacing this prefab's
	* current data. Returns whether reading succeeded.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* path（JSON）から取得済みデータを読み込み、この prefab の現在の
	* データを置き換える。読み込みに成功したかどうかを返す。
	*/
	Bool PrefabBlueprint::Read(const std::filesystem::path& path)
	{
		JsonInputArchive archive;
		if (!archive.Read(String(path.string())))
		{
			SC_LOG_ERROR("Prefabの読み込みに失敗しました ({})", path.string());
			return false;
		}

		Load(archive);

		return true;
	}
}
