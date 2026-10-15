#include <FoundationEngine/Resource/Scene/SceneBlueprint.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/Log/Error.h>
#include <FoundationEngine/Serialization/Json/JsonArchive.h>

namespace SeedCore
{
	/**
	* [EN]
	* Records every root actor (and its descendants) in world into
	* nodes_, replacing any previously captured data.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world 内の全ルート actor（とその子孫）を nodes_ へ記録し、以前に
	* 取得していたデータを置き換える。
	*/
	void SceneBlueprint::Capture(World& world)
	{
		nodes_.clear();

		/// [EN] Only root actors are enumerated here; CaptureActorNode recurses into each root's descendants on its own.
		/// [JP] ここで列挙するのはルート actor のみ。各ルートの子孫への再帰は CaptureActorNode 自身が行う。
		for (Actor actor : world.GetActors())
		{
			if (!actor.Parent())
			{
				CaptureActorNode(actor, -1, nodes_);
			}
		}
	}

	/**
	* [EN]
	* Discards any captured actor data, releasing the memory it held.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 取得済みの actor データを破棄し、それが保持していたメモリを解放する。
	*/
	void SceneBlueprint::Clear()
	{
		nodes_.clear();
		nodes_.shrink_to_fit();
	}

	/**
	* [EN]
	* Recreates the captured scene as new actors in world, returning
	* every instantiated actor (roots and descendants alike).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 取得済みのシーンを world 内に新しい actor 群として再生成し、
	* インスタンス化された全 actor（ルート・子孫問わず）を返す。
	*/
	DynamicArray<Actor> SceneBlueprint::Instantiate(World& world, ResourceCache& cache)const
	{
		DynamicArray<Actor> instantiated;
		instantiated.reserve(nodes_.size());

		/// [EN] Nodes are stored in capture order, so each node's parent (identified by an earlier index) has always already been instantiated by the time we reach it.
		/// [JP] ノードは取得順に格納されているため、各ノードの親（それより前のインデックスで識別される）は、そこに到達する時点で既にインスタンス化済みである。
		for (Size index = 0; index < nodes_.size(); ++index)
		{
			const BlueprintNode& node = nodes_[index];

			Actor parentActor;
			if (node.parentIndex_ >= 0 && static_cast<Size>(node.parentIndex_) < instantiated.size())
			{
				parentActor = instantiated[node.parentIndex_];
			}

			Actor actor = InstantiateActorNode(world, cache, node, parentActor, false);
			instantiated.push_back(actor);
		}

		return instantiated;
	}

	/**
	* [EN]
	* Writes this scene's captured data to path as JSON. Returns whether
	* the file was written successfully.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このシーンの取得済みデータを、JSON として path へ書き込む。
	* ファイルが正常に書き込まれたかどうかを返す。
	*/
	Bool SceneBlueprint::Write(const std::filesystem::path& path)
	{
		JsonOutputArchive archive;
		Save(archive);
		return archive.Write(String(path.string()));
	}

	/**
	* [EN]
	* Reads captured data from path (JSON), replacing this scene's
	* current data, and takes path's file name without extension as this
	* scene's name. Returns whether reading succeeded.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* path（JSON）から取得済みデータを読み込み、このシーンの現在の
	* データを置き換える。path の拡張子を除いたファイル名を、この
	* シーンの名前とする。読み込みに成功したかどうかを返す。
	*/
	Bool SceneBlueprint::Read(const std::filesystem::path& path)
	{
		JsonInputArchive archive;
		if (!archive.Read(String(path.string())))
		{
			SC_LOG_ERROR("シーンの読み込みに失敗しました ({})", path.string());
			return false;
		}

		Load(archive);
		name_ = String(path.stem().string());

		return true;
	}

	/**
	* [EN]
	* Returns how this scene wants to be rendered, as captured by the
	* last Read(). SceneBlueprint itself knows nothing about what these
	* strings hold - FoundationEngine cannot depend on GraphicsEngine -
	* so the renderer is what produces and reads them back.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この Scene をどう描くかを、直近の Read() で取得した内容として
	* 返す。SceneBlueprint 自体はこの文字列の中身を知らない
	* （FoundationEngine は GraphicsEngine に依存できない）ため、作るのも
	* 読み戻すのもレンダラ側になる。
	*/
	const SceneVisual& SceneBlueprint::Visual()const
	{
		return visual_;
	}

	/**
	* [EN]
	* Returns the captured actors of the last Read()/Capture(), in the
	* order they were laid out. Used when a scene that arrived from
	* another member has to be matched against the actors already live
	* in the world rather than instantiated from scratch.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直近の Read()/Capture() で取得した actor を、並んでいた順のまま
	* 返す。他のメンバーから届いた Scene を、新規にインスタンス化する
	* のではなく、既に world にいる actor と突き合わせる際に使う。
	*/
	const DynamicArray<BlueprintNode>& SceneBlueprint::Nodes()const
	{
		return nodes_;
	}

	/**
	* [EN]
	* Returns this scene's name: the file name without extension it was
	* read from, or the name SceneSystem gave it when capturing a world.
	* Empty if neither has happened.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このシーンの名前を返す。読み込み元の拡張子を除いたファイル名か、
	* SceneSystem が world を取得した際に付けた名前。どちらも無ければ空。
	*/
	const String& SceneBlueprint::Name()const
	{
		return name_;
	}
}
