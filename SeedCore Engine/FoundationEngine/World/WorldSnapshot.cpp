#include <FoundationEngine/World/WorldSnapshot.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

namespace SeedCore
{
	/**
	* [EN]
	* Clears any existing snapshot, then captures a copy of every
	* Actor's component data currently in world.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 既存のスナップショットをクリアし、world 内の現在の全 Actor の
	* コンポーネントデータのコピーを取得する。
	*/
	void WorldSnapshot::Capture(World& world)
	{
		Clear();

		ActorRegistry& actors = WorldAccessor::Actors(world);
		ArchetypeStorage& archetypes = WorldAccessor::Archetypes(world);
		EntityRegistry& entities = WorldAccessor::Entities(world);

		/// [EN] Walk every actor in hierarchy order: each scene root followed by its descendants, depth first.
		/// [JP] 全 actor を階層の順で回る。各シーンのルートの後に、その子孫が深さ優先で続く。
		for (EntityID current = actors.Root(); current != EntityID{}; current = actors.Next(current))
		{
			ActorData actorData;
			actorData.entity_ = current;
			Entity entity = Entity(Handle<Entity>{ current.index_, current.generation_ });

			/// [EN] Copy every archetype-stored component in the actor's layout into a freshly-allocated byte buffer, via the component's own copy_ function.
			/// [JP] actor のレイアウト内の全アーキタイプ格納コンポーネントを、そのコンポーネント自身の copy_ 関数経由で、新規確保したバイトバッファへコピーする。
			for (ComponentID id : archetypes.Layout(current))
			{
				void* source = world.GetComponent(entity, id);
				if (!source)
				{
					continue;
				}

				const ComponentMetadata& meta = ComponentRegistry::Get(id);
				ComponentData componentData;
				componentData.id_ = id;
				componentData.data_.resize(meta.size_);
				meta.copy_(componentData.data_.data(), source);
				actorData.components_.push_back(std::move(componentData));
			}

			/// [EN] The entity lists its own sparse-stored components (Rigidbody, Softbody, Weather...), so their runtime values are restored on Stop as well; the behaviours among them are remembered for resetting their lifecycle.
			/// [JP] エンティティは自分のスパース格納コンポーネント(Rigidbody、Softbody、Weather など)を一覧で持っているので、実行中に変わった値も Stop で元に戻せる。その中のビヘイビアは、ライフサイクルを戻すために覚えておく。
			for (const SparseSetLocation& location : entities.Record(current)->sparseSet_)
			{
				const ComponentMetadata& meta = ComponentRegistry::Get(location.id_);
				ComponentData componentData;
				componentData.id_ = location.id_;
				componentData.data_.resize(meta.size_);
				meta.copy_(componentData.data_.data(), location.data_);
				actorData.components_.push_back(std::move(componentData));

				if (location.id_->hooks_ && location.id_->hooks_->upcast_)
				{
					actorData.componentBaseIDs_.push_back(location.id_);
				}
			}

			actors_.push_back(std::move(actorData));
		}

		hasData_ = true;
	}

	/**
	* [EN]
	* If a snapshot has been captured, copies its component data back
	* onto world's actors (matched by EntityID) and resets each
	* ComponentBehaviour's lifecycle state, destroys every actor the
	* snapshot does not know (i.e. created after it, such as by
	* SpawnerSystem during Play), then clears the snapshot. Does nothing
	* if no snapshot exists.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スナップショットが取得済みであれば、そのコンポーネントデータを
	* （EntityID で対応付けた）world の actor 群へコピーし戻し、各
	* ComponentBehaviour のライフサイクル状態をリセットし、スナップショットが
	* 知らない actor（それより後に生成されたもの、例えば Play 中の
	* SpawnerSystem による生成）を全て破棄した上で、スナップショットを
	* クリアする。スナップショットが存在しなければ何もしない。
	*/
	void WorldSnapshot::Restore(World& world)
	{
		if (!hasData_)
		{
			return;
		}

		ActorRegistry& actors = WorldAccessor::Actors(world);
		EntityRegistry& entities = WorldAccessor::Entities(world);

		/// [EN] The captured entity of each slot, so an actor can be told known or new by one array access.
		/// [JP] スロットごとの取得済みエンティティ。actor が既知か新規かを、配列1回で見分けられるようにする。
		DynamicArray<EntityID> captured;

		for (const ActorData& actorData : actors_)
		{
			if (actorData.entity_.index_ >= captured.size())
			{
				captured.resize(actorData.entity_.index_ + 1);
			}
			captured[actorData.entity_.index_] = actorData.entity_;

			/// [EN] An actor destroyed during Play cannot be brought back here, so it is skipped.
			/// [JP] Play 中に破棄された actor はここでは戻せないので飛ばす。
			if (!entities.Exists(actorData.entity_))
			{
				continue;
			}
			Entity entity = Entity(Handle<Entity>{ actorData.entity_.index_, actorData.entity_.generation_ });

			/// [EN] Dispatch OnDestroy to every ComponentBehaviour-derived component first, while its live data (e.g. a Rigidbody's JPH body/shape handles) is still intact, so play-mode-only resources are released before the snapshot bytes overwrite it.
			/// [JP] ComponentBehaviour 派生の全コンポーネントへ、まず OnDestroy をディスパッチする。生きたデータ（Rigidbody の JPH ボディ/シェイプハンドルなど）がまだ有効なうちに、プレイモード限定のリソースをスナップショットのバイト列で上書きする前に解放するため。
			for (ComponentID id : actorData.componentBaseIDs_)
			{
				if (void* data = world.GetComponent(entity, id))
				{
					id->hooks_->upcast_(data)->DispatchDestroy();
				}
			}

			/// [EN] Overwrite each current component in place with the captured bytes: destruct the live value first, then copy-construct from the snapshot.
			/// [JP] 現在の各コンポーネントを、取得済みのバイト列でその場で上書きする: まず現在の値を破棄し、その後スナップショットからコピー構築する。
			for (const ComponentData& componentData : actorData.components_)
			{
				void* dest = world.GetComponent(entity, componentData.id_);
				if (!dest)
				{
					continue;
				}

				const ComponentMetadata& meta = ComponentRegistry::Get(componentData.id_);
				meta.destruct_(dest);
				meta.copy_(dest, componentData.data_.data());
			}

			/// [EN] Reset Awake/Start flags on every ComponentBehaviour-derived component, so they fire again on the next frame as if freshly attached.
			/// [JP] 全ての ComponentBehaviour 派生コンポーネントの Awake/Start フラグをリセットし、次のフレームで新規アタッチされたかのように再び発火するようにする。
			for (ComponentID id : actorData.componentBaseIDs_)
			{
				if (void* data = world.GetComponent(entity, id))
				{
					id->hooks_->upcast_(data)->ResetLifecycle();
				}
			}
		}

		/// [EN] Actors the snapshot does not know were created during Play (e.g. by SpawnerSystem); they are collected first and destroyed afterwards, since destroying changes the hierarchy being walked.
		/// [JP] スナップショットが知らない actor は Play 中に生まれたもの(例: SpawnerSystem)。破棄すると回している階層が変わるので、先に集めてから後で破棄する。
		DynamicArray<Actor> created;
		for (EntityID current = actors.Root(); current != EntityID{}; current = actors.Next(current))
		{
			if (current.index_ >= captured.size() || captured[current.index_] != current)
			{
				created.push_back(WorldAccessor::Make(world, current));
			}
		}

		for (Actor actor : created)
		{
			world.DestroyActor(actor);
		}

		Clear();
	}

	/**
	* [EN]
	* Returns whether a snapshot is currently held.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スナップショットを現在保持しているかどうかを返す。
	*/
	Bool WorldSnapshot::HasData()const
	{
		return hasData_;
	}

	/**
	* [EN]
	* Destructs and discards all captured component data.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 取得済みの全コンポーネントデータを破棄・削除する。
	*/
	void WorldSnapshot::Clear()
	{
		/// [EN] Destruct every captured component's live representation in its byte buffer before releasing the buffers themselves, to avoid leaking any owned resources (e.g. Strings) inside them.
		/// [JP] バッファ自体を解放する前に、取得済みの各コンポーネントのバイトバッファ内にある有効な表現を破棄する。中に保持されているリソース（String など）のリークを防ぐため。
		for (ActorData& actorData : actors_)
		{
			for (ComponentData& componentData : actorData.components_)
			{
				const ComponentMetadata& meta = ComponentRegistry::Get(componentData.id_);
				meta.destruct_(componentData.data_.data());
			}
		}
		actors_.clear();
		hasData_ = false;
	}
}
