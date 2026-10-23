#include <FoundationEngine/Resource/Blueprint/BlueprintNode.h>
#include <FoundationEngine/Resource/Prefab/PrefabBlueprint.h>
#include <FoundationEngine/Resource/Prefab/PrefabRegistry.h>
#include <FoundationEngine/Resource/Sharing/CollaborationRegistry.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/Tag/TagRegistry.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/ECS/Component/Name.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Component/WorldMatrix.h>
#include <FoundationEngine/World/ECS/Component/Bounds.h>
#include <FoundationEngine/World/ECS/Component/Velocity.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/World/ECS/Component/UnknownComponent.h>

namespace SeedCore
{
	/**
	* [EN]
	* Writes this node onto actor, which already exists: adds the
	* components the node has, removes the ones it no longer has, and
	* restores every field, the name, transform, active state, layer and
	* tags. A nested prefab instance's node carries no components, so for
	* one the components are left as the prefab built them.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノードを、既に存在する actor へ書き込む: ノードが持つ
	* コンポーネントを追加し、持たなくなったものを削除し、各フィールド・
	* 名前・トランスフォーム・アクティブ状態・レイヤー・タグを復元する。
	* ネストされたプレハブインスタンスのノードはコンポーネントを持たない
	* ので、その場合コンポーネントはプレハブが作ったままにする。
	*/
	void BlueprintNode::Apply(Actor actor)const
	{
		if (!actor)
		{
			return;
		}

		World& world = actor.GetWorld();
		EntityID entityID = ActorAccessor::GetEntity(actor).GetID();

		if (nestedPrefabAssetID_ == 0)
		{
			/// [EN] The actor's own component list is copied first, since removing a component moves the actor to another archetype and the layout it was read from no longer holds.
			/// [JP] actor 自身のコンポーネントの一覧を先に写す。コンポーネントを外すと actor は別のアーキタイプへ移り、読んだレイアウトはもう当てにならないため。
			const DynamicArray<ComponentID>& layout = WorldAccessor::Archetypes(world).Layout(entityID);
			DynamicArray<ComponentID> owned(layout.begin(), layout.end());
			for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
			{
				owned.push_back(location.id_);
			}

			/// [EN] A component the node no longer carries was removed by whoever wrote the node, so it goes here too; built-in ones are never captured, so they are never removed.
			/// [JP] ノードが持たなくなったコンポーネントは、ノードを書いた側が削除したということ。ここでも同じように消す。組み込みのものはそもそも取得しないので、消すこともない。
			for (ComponentID id : owned)
			{
				if (Builtin(id) || std::ranges::contains(components_, ComponentRegistry::Name(id), &BlueprintComponent::componentName_))
				{
					continue;
				}
				actor.RemoveComponent(id);
			}

			/// [EN] Everything the node does carry is added when missing (AddComponent returns the existing one otherwise) and refilled either way.
			/// [JP] ノードが持つものは、無ければ追加し（あれば AddComponent が既存のものを返す）、いずれにせよ中身を入れ直す。
			for (const BlueprintComponent& component : components_)
			{
				/// [EN] A component whose type is not registered (a script not built on this machine yet) is kept as it was saved rather than dropped.
				/// [JP] 型が登録されていないコンポーネント（この PC ではまだビルドしていないスクリプトなど）は、捨てずに保存されたまま保持する。
				ComponentID id = ComponentRegistry::GetComponentID(component.componentName_);
				if (!id)
				{
					UnknownComponent::Keep(actor, component);
					continue;
				}

				void* componentData = actor.AddComponent(id);
				if (componentData)
				{
					component.Apply(componentData);
				}
			}
		}

		Name* name = actor.GetComponent<Name>();
		if (name)
		{
			name->name_ = name_;
		}

		Position* position = actor.GetComponent<Position>();
		if (position)
		{
			Transform::Vector(*position, position_);
		}

		Rotation* rotation = actor.GetComponent<Rotation>();
		if (rotation)
		{
			Transform::Quat(*rotation, rotation_);
		}

		Scale* scale = actor.GetComponent<Scale>();
		if (scale)
		{
			Transform::Vector(*scale, scale_);
		}

		actor.Active(active_);
		actor.Layer(layerName_);

		/// [EN] Tags are replaced rather than merged: every bit is cleared at once, then the node's tags are set again.
		/// [JP] タグは統合ではなく置き換える。全てのビットを一度に消してから、ノードのタグを付け直す。
		ActorRecord* record = WorldAccessor::Actors(world).Record(entityID);
		record->tags_ = 0;
		record->extraTags_ = nullptr;
		for (const String& tag : tags_)
		{
			actor.AddTag(tag);
		}
	}

	/**
	* [EN]
	* Appends root and every descendant to nodes as a flat,
	* parent-index-linked array, root first with parentIndex_ -1.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* root とその全子孫を、フラットな親インデックス連結配列として nodes
	* の末尾へ追加する。root が先頭で、その parentIndex_ は -1。
	*/
	void BlueprintNode::Capture(Actor root, DynamicArray<BlueprintNode>& nodes)
	{
		Capture(root, -1, nodes);
	}

	/**
	* [EN]
	* Recreates every node in nodes as a live actor in world, in order,
	* and returns them in the same order. Each node's parent is the actor
	* made from the node its parentIndex_ names; a root goes under parent.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* nodes の各ノードを、順に world 内の生きた actor として作り直し、同じ
	* 順で返す。各ノードの親は、parentIndex_ が示すノードから作った
	* actor。ルートは parent の下に入る。
	*/
	DynamicArray<Actor> BlueprintNode::Instantiate(World& world, ResourceCache& cache, const DynamicArray<BlueprintNode>& nodes, Actor parent, Bool fromPrefab)
	{
		DynamicArray<Actor> actors;
		actors.reserve(nodes.size());

		/// [EN] Nodes are stored in capture order, so a node's parent (always an earlier index) has already been made by the time the node is reached.
		/// [JP] ノードは取得順に並んでいるので、ノードの親（必ずそれより前のインデックス）は、そのノードに着いた時点で既に作られている。
		for (const BlueprintNode& node : nodes)
		{
			Actor nodeParent = parent;
			if (node.parentIndex_ >= 0 && static_cast<Size>(node.parentIndex_) < actors.size())
			{
				nodeParent = actors[node.parentIndex_];
			}
			actors.push_back(node.Instantiate(world, cache, nodeParent, fromPrefab));
		}

		return actors;
	}

	/**
	* [EN]
	* Recreates this node alone as a live actor in world under parent,
	* by instantiating the referenced nested prefab or creating a fresh
	* actor, then applying the node onto it. Returns an invalid Actor if a
	* nested prefab could not be instantiated.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このノード1つを、world 内の parent の下に生きた actor として作り
	* 直す。参照されているネストされたプレハブをインスタンス化するか、
	* 新しい actor を作るかしてから、ノードを書き込む。ネストされた
	* プレハブをインスタンス化できなければ無効な Actor を返す。
	*/
	Actor BlueprintNode::Instantiate(World& world, ResourceCache& cache, Actor parent, Bool fromPrefab)const
	{
		Actor actor;
		Service& services = WorldAccessor::Services(world);

		if (nestedPrefabAssetID_ != 0)
		{
			/// [EN] A nested prefab reference: the referenced prefab builds the actor and its subtree; this node only supplies what the outer scene or prefab changed on top.
			/// [JP] ネストされたプレハブへの参照: actor とそのサブツリーは参照先のプレハブが作る。このノードが与えるのは、外側のシーンやプレハブがその上で変えた部分だけ。
			Handle<PrefabBlueprint> nestedHandle = cache.GetPrefabPool().Load(nestedPrefabAssetID_, cache);
			PrefabBlueprint* nestedPrefab = cache.GetPrefabPool().Get(nestedHandle);
			if (nestedPrefab)
			{
				actor = nestedPrefab->Instantiate(world, cache, parent, nestedPrefabAssetID_);
			}

			if (!actor)
			{
				return Actor();
			}
		}
		else
		{
			actor = WorldAccessor::Create(world, name_, persistentId_);

			/// [EN] Only prefab-built actors take up a record, so a world that never instantiates a prefab never creates the registry.
			/// [JP] レコードを使うのはプレハブ由来の actor だけなので、プレハブを一度もインスタンス化しないワールドではレジストリ自体が作られない。
			if (fromPrefab)
			{
				services.Add<PrefabRegistry>()->Add(ActorAccessor::GetEntity(actor).GetID(), 0, true);
			}
		}

		/// [EN] A prefab's actors get new identities in every instance, so only actors loaded from a scene keep their collaboration identifier.
		/// [JP] プレハブの actor はインスタンスごとに別物になるので、共同編集の識別子を保つのはシーンから読み込んだ actor だけ。
		if (!fromPrefab && collaborationId_ != String{})
		{
			services.Add<CollaborationRegistry>()->Add(ActorAccessor::GetEntity(actor).GetID(), collaborationId_);
		}

		Apply(actor);

		if (parent)
		{
			actor.Parent(parent);
		}

		return actor;
	}

	/**
	* [EN]
	* Returns whether id is one of the components the engine attaches on
	* its own rather than the node capturing it: those every actor gets on
	* creation (Name, Position, Rotation, Scale, Active, WorldMatrix,
	* Bounds) and the velocities Rigidbody attaches with itself
	* (LinearVelocity, AngularVelocity).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* id が、ノードが取得するのではなく、エンジンが自分で付けるコンポーネント
	* のどれかかどうかを返す。全ての actor に生成時に付くもの（Name、
	* Position、Rotation、Scale、Active、WorldMatrix、Bounds）と、Rigidbody
	* が自分と一緒に付ける速度（LinearVelocity、AngularVelocity）。
	*/
	Bool BlueprintNode::Builtin(ComponentID id)
	{
		return id == ComponentRegistry::GetComponentID<Name>() || id == ComponentRegistry::GetComponentID<Position>() || id == ComponentRegistry::GetComponentID<Rotation>() || id == ComponentRegistry::GetComponentID<Scale>() || id == ComponentRegistry::GetComponentID<Active>() || id == ComponentRegistry::GetComponentID<WorldMatrix>() || id == ComponentRegistry::GetComponentID<Bounds>() || id == ComponentRegistry::GetComponentID<LinearVelocity>() || id == ComponentRegistry::GetComponentID<AngularVelocity>();
	}

	/**
	* [EN]
	* Appends actor to nodes with parentIndex as its parent, then its
	* descendants after it, depth first, and returns actor's own index
	* within nodes. A non-root actor that is a nested prefab instance's
	* root records only the prefab reference.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* actor を、parentIndex を親として nodes へ追加し、その後に子孫を
	* 深さ優先で続け、nodes 内での actor 自身のインデックスを返す。
	* ネストされたプレハブインスタンスのルートである非ルートの actor は、
	* プレハブへの参照だけを記録する。
	*/
	Int BlueprintNode::Capture(Actor actor, Int parentIndex, DynamicArray<BlueprintNode>& nodes)
	{
		World& world = actor.GetWorld();
		EntityID entityID = ActorAccessor::GetEntity(actor).GetID();
		ActorRegistry& actors = WorldAccessor::Actors(world);
		const ActorRecord* record = actors.Record(entityID);
		Service& services = WorldAccessor::Services(world);

		BlueprintNode node;

		const Name* name = actor.GetComponent<Name>();
		node.name_ = name ? name->name_ : String("Actor");

		TagRegistry::ForEach(*record, [&node](const String& tag)
			{
				node.tags_.push_back(tag);
			});
		node.layerName_ = actor.LayerName();
		node.active_ = actor.Active();
		node.persistentId_ = record->persistentID_;

		const CollaborationRegistry* collaborations = services.Get<CollaborationRegistry>();
		const CollaborationRecord* collaboration = collaborations ? collaborations->Record(entityID) : nullptr;
		node.collaborationId_ = collaboration ? collaboration->id_ : String{};

		const Position* position = actor.GetComponent<Position>();
		if (position)
		{
			node.position_ = Vector3(position->x_, position->y_, position->z_);
		}

		const Rotation* rotation = actor.GetComponent<Rotation>();
		if (rotation)
		{
			node.rotation_ = Transform::Quat(*rotation);
		}

		const Scale* scale = actor.GetComponent<Scale>();
		if (scale)
		{
			node.scale_ = Vector3(scale->x_, scale->y_, scale->z_);
		}

		node.parentIndex_ = parentIndex;

		/// [EN] A non-root actor whose own subtree came from a prefab records only the prefab reference, so instantiating it again stays in sync with the source prefab.
		/// [JP] 自身のサブツリーがプレハブ由来である非ルートの actor は、プレハブへの参照だけを記録する。これにより、再びインスタンス化したときに元のプレハブと同期した状態を保つ。
		const PrefabRegistry* prefabs = services.Get<PrefabRegistry>();
		const PrefabRecord* prefab = prefabs ? prefabs->Record(entityID) : nullptr;
		Uint32 prefabID = prefab ? prefab->assetID_ : 0;
		Bool isNestedInstance = (parentIndex != -1) && (prefabID != 0);

		if (isNestedInstance)
		{
			node.nestedPrefabAssetID_ = prefabID;
		}
		else
		{
			/// [EN] The archetype layout lists every archetype-stored component; the built-in ones are skipped, since the node holds them as dedicated members.
			/// [JP] アーキタイプのレイアウトは、アーキタイプ格納の全コンポーネントを並べる。組み込みのものは、ノードが専用のメンバーとして持つので飛ばす。
			for (ComponentID id : WorldAccessor::Archetypes(world).Layout(entityID))
			{
				if (Builtin(id))
				{
					continue;
				}
				node.components_.push_back(BlueprintComponent::Capture(ComponentRegistry::Name(id), actor.GetComponent(id)));
			}

			/// [EN] The entity's sparse locations list every sparse-stored component, behaviours and plain data alike, with their data already at hand.
			/// [JP] エンティティのスパースの場所は、ビヘイビアもただのデータも含めた全てのスパース格納コンポーネントを、データ付きで並べる。
			ComponentID unknownID = ComponentRegistry::GetComponentID<UnknownComponent>();
			for (const SparseSetLocation& location : WorldAccessor::Entities(world).Record(entityID)->sparseSet_)
			{
				/// [EN] The holder of unknown components is not saved as itself: what it holds is written back exactly as it was read, so saving here never drops another member's script.
				/// [JP] 型の分からないコンポーネントの保持役は、それ自体としては保存しない。保持している中身を読み込んだときのまま書き戻すので、ここで保存しても他のメンバーのスクリプトを落とすことはない。
				if (location.id_ == unknownID)
				{
					const UnknownComponent* unknown = static_cast<const UnknownComponent*>(location.data_);
					node.components_.insert(node.components_.end(), unknown->components_.begin(), unknown->components_.end());
					continue;
				}
				node.components_.push_back(BlueprintComponent::Capture(ComponentRegistry::Name(location.id_), location.data_));
			}
		}

		Int index = static_cast<Int>(nodes.size());
		nodes.push_back(std::move(node));

		/// [EN] Children are reached through the sibling links directly, so no array of them is built.
		/// [JP] 子は兄弟のつながりを直接たどるので、子の配列は作らない。
		if (!isNestedInstance)
		{
			for (EntityID child = record->child_; child != EntityID{}; child = actors.Record(child)->nextSibling_)
			{
				Capture(WorldAccessor::Make(world, child), index, nodes);
			}
		}

		return index;
	}
}
