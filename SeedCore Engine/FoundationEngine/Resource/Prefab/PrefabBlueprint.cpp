#include <FoundationEngine/Resource/Prefab/PrefabBlueprint.h>
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
		CaptureActorNode(root, -1, nodes_);
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
		DynamicArray<Actor> instantiated;
		instantiated.reserve(nodes_.size());

		/// [EN] Nodes are stored in capture order, so each node's parent (identified by an earlier index) has always already been instantiated by the time we reach it.
		/// [JP] ノードは取得順に格納されているため、各ノードの親（それより前のインデックスで識別される）は、そこに到達する時点で既にインスタンス化済みである。
		for (Size index = 0; index < nodes_.size(); ++index)
		{
			const PrefabNode& node = nodes_[index];

			Actor parentActor;
			if (node.parentIndex_ >= 0 && static_cast<Size>(node.parentIndex_) < instantiated.size())
			{
				parentActor = instantiated[node.parentIndex_];
			}
			else if (index == 0)
			{
				/// [EN] The root node has no in-subtree parent: attach it to the caller-supplied parent instead.
				/// [JP] ルートノードはサブツリー内に親を持たない: 代わりに呼び出し側が指定した parent へアタッチする。
				parentActor = parent;
			}

			Actor actor = InstantiateActorNode(world, cache, node, parentActor, true);

			if (actor && index == 0)
			{
				actor.PrefabID(sourceAssetID);
			}

			instantiated.push_back(actor);
		}

		return instantiated.empty() ? Actor() : instantiated[0];
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
