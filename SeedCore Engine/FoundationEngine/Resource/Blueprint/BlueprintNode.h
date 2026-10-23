#pragma once
#include <FoundationEngine/Resource/Blueprint/BlueprintComponent.h>
#include <FoundationEngine/World/Actor/Actor.h>

namespace SeedCore
{
	class Actor;
	class World;
	class ResourceCache;

	/**
	* [EN]
	* Serialized snapshot of a single Actor within a Scene or Prefab:
	* its name/tags/active state/transform, every non-builtin component
	* it holds, its position within the flat parent-index-linked node
	* array, and (if it's actually the root of a nested prefab
	* instance) a reference back to that prefab asset instead of its
	* own component list.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Scene または Prefab 内の単一 Actor のシリアライズ済みスナップショット。
	* その名前/タグ/アクティブ状態/トランスフォーム、保持している全ての
	* 非組み込みコンポーネント、フラットな親インデックス連結ノード配列内
	* での位置、および（実際にネストされたプレハブインスタンスのルートで
	* あれば）自身のコンポーネント一覧の代わりに、そのプレハブアセットへの
	* 参照。
	*/
	struct BlueprintNode
	{
		/// [EN] The actor's display name.
		/// [JP] actor の表示名。
		String name_;

		/// [EN] Every tag set on the actor.
		/// [JP] actor に設定されている全タグ。
		DynamicArray<String> tags_;

		/// [EN] Name of the actor's layer at capture time (see LayerRegistry). Stored by name, not index, so a slot reshuffle elsewhere doesn't retarget this actor to the wrong layer.
		/// [JP] 取得時点での actor のレイヤー名（LayerRegistry 参照）。インデックスではなく名前で保存する。他所でのスロット並び替えにより、この actor が別のレイヤーへ誤って向いてしまわないようにするため。
		String layerName_;

		/// [EN] Whether the actor was active at capture time.
		/// [JP] 取得時点で actor がアクティブだったかどうか。
		Bool active_ = true;

		/// [EN] Local position at capture time.
		/// [JP] 取得時点でのローカル位置。
		Vector3 position_ = Vector3(0.0f, 0.0f, 0.0f);

		/// [EN] Local rotation at capture time, as a normalized quaternion.
		/// [JP] 取得時点でのローカル回転。正規化済みクォータニオン。
		Quaternion rotation_ = Quaternion::Identity;

		/// [EN] Local scale at capture time.
		/// [JP] 取得時点でのローカルスケール。
		Vector3 scale_ = Vector3(1.0f, 1.0f, 1.0f);

		/// [EN] Every non-builtin component captured for this actor (empty if this node is a nested prefab instance; see nestedPrefabAssetID_).
		/// [JP] この actor について取得済みの、全非組み込みコンポーネント（このノードがネストされたプレハブインスタンスであれば空。nestedPrefabAssetID_ を参照）。
		DynamicArray<BlueprintComponent> components_;

		/// [EN] Index of this node's parent within the owning array, or -1 if it's a root.
		/// [JP] 所有元の配列内における、このノードの親のインデックス。ルートであれば -1。
		Int parentIndex_ = -1;

		/// [EN] The actor's persistent ID at capture time (see Actor::PersistentID), restored on instantiation so other actors (e.g. constraint targets) can reference it stably. 0 if never assigned.
		/// [JP] 取得時点での actor の永続ID（Actor::PersistentID 参照）。インスタンス化時に復元され、他の actor（例: コンストレイントのターゲット）がこれを安定して参照できるようにする。未割り当てなら 0。
		Uint32 persistentId_ = 0;

		/// [EN] The identifier every member's copy of the scene shares for this actor (see CollaborationRegistry); empty if it has none.
		/// [JP] 各メンバーのシーンで共通の、この actor の識別子（CollaborationRegistry 参照）。持っていなければ空。
		String collaborationId_;

		/// [EN] If nonzero, this node is the root of a nested prefab instance: instantiate that prefab asset instead of using components_.
		/// [JP] 0以外であれば、このノードはネストされたプレハブインスタンスのルートである: components_ を使う代わりに、そのプレハブアセットをインスタンス化する。
		Uint32 nestedPrefabAssetID_ = 0;

		/// [EN] Field-value overrides applied on top of a nested prefab instance (currently reserved; see nestedPrefabAssetID_).
		/// [JP] ネストされたプレハブインスタンスの上に適用される、フィールド値の上書き（現時点では予約済み。nestedPrefabAssetID_ を参照）。
		DynamicArray<BlueprintField> overrides_;

		/**
		* [EN]
		* Serialization hook (save side): writes every member above.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シリアライズ用フック(保存側): 上記の全メンバを書き込む。
		*/
		template<class Archive>
		void Save(Archive& archive)const
		{
			archive.Field("name", name_);
			archive.Field("tags", tags_);
			archive.Field("layer", layerName_);
			archive.Field("active", active_);
			archive.Field("position", position_);
			archive.Field("rotation", rotation_);
			archive.Field("scale", scale_);
			archive.Field("components", components_);
			archive.Field("parentIndex", parentIndex_);
			archive.Field("nestedPrefabAssetID", nestedPrefabAssetID_);
			archive.Field("overrides", overrides_);
			archive.Field("persistentId", persistentId_);
			archive.Field("collaborationId", collaborationId_);
		}

		/**
		* [EN]
		* Serialization hook (load side): reads every member above.
		* persistentId_ is optional -- nodes saved before this field
		* existed simply leave it 0 (World::CreateActor auto-allocates a
		* fresh ID in that case) -- so a missing key is swallowed rather
		* than failing the whole load.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シリアライズ用フック(読み込み側): 上記の全メンバを読み込む。
		* persistentId_ は任意項目 -- このフィールドが存在する前に保存された
		* ノードでは単に 0 のままになる(その場合 World::CreateActor が
		* 新規IDを自動割り当てする) -- なので、キーが無くても読み込み全体を
		* 失敗させず読み飛ばす。
		*/
		template<class Archive>
		void Load(Archive& archive)
		{
			archive.TryField("name", name_);
			archive.TryField("tags", tags_);
			archive.TryField("layer", layerName_);
			archive.TryField("active", active_);
			archive.TryField("position", position_);
			archive.TryField("rotation", rotation_);
			archive.TryField("scale", scale_);
			archive.TryField("components", components_);
			archive.TryField("parentIndex", parentIndex_);
			archive.TryField("nestedPrefabAssetID", nestedPrefabAssetID_);
			archive.TryField("overrides", overrides_);
			archive.TryField("persistentId", persistentId_);
			archive.TryField("collaborationId", collaborationId_);
		}

		/**
		* [EN]
		* Writes this node onto actor, which already exists: adds the
		* components the node has, removes the ones it no longer has, and
		* restores every field, the name, transform, active state, layer and
		* tags. A nested prefab instance's node carries no components, so
		* for one the components are left as the prefab built them. The
		* actor keeps its identity, its children and its place in the
		* hierarchy.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノードを、既に存在する actor へ書き込む: ノードが持つ
		* コンポーネントを追加し、持たなくなったものを削除し、各フィールド・
		* 名前・トランスフォーム・アクティブ状態・レイヤー・タグを復元する。
		* ネストされたプレハブインスタンスのノードはコンポーネントを持たない
		* ので、その場合コンポーネントはプレハブが作ったままにする。actor の
		* 同一性・子・階層内の位置は保たれる。
		*/
		SEEDCORE_API void Apply(Actor actor)const;

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
		SEEDCORE_API static void Capture(Actor root, DynamicArray<BlueprintNode>& nodes);

		/**
		* [EN]
		* Recreates every node in nodes as a live actor in world, in order,
		* and returns them in the same order. Each node's parent is the actor
		* made from the node its parentIndex_ names; a node with none (a
		* root) goes under parent, or becomes a scene root if parent is
		* invalid.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* nodes の各ノードを、順に world 内の生きた actor として作り直し、同じ
		* 順で返す。各ノードの親は、parentIndex_ が示すノードから作った
		* actor。それが無いノード（ルート）は parent の下に入り、parent が
		* 無効ならシーンのルートになる。
		*/
		SEEDCORE_API static DynamicArray<Actor> Instantiate(World& world, ResourceCache& cache, const DynamicArray<BlueprintNode>& nodes, Actor parent, Bool fromPrefab);

		/**
		* [EN]
		* Recreates this node alone as a live actor in world under parent:
		* by instantiating the referenced nested prefab, or by creating a
		* fresh actor, then applying the node onto it. Records the actor as
		* prefab-built when fromPrefab is set, and its collaboration
		* identifier otherwise. Returns an invalid Actor if a nested prefab
		* could not be instantiated.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このノード1つを、world 内の parent の下に生きた actor として作り
		* 直す: 参照されているネストされたプレハブをインスタンス化するか、
		* 新しい actor を作るかしてから、ノードを書き込む。fromPrefab なら
		* その actor をプレハブ由来として記録し、そうでなければ共同編集の
		* 識別子を記録する。ネストされたプレハブをインスタンス化できなければ
		* 無効な Actor を返す。
		*/
		SEEDCORE_API Actor Instantiate(World& world, ResourceCache& cache, Actor parent, Bool fromPrefab)const;

	private:
		/**
		* [EN]
		* Returns whether id is one of the components the engine attaches on
		* its own rather than the node capturing it: those every actor gets
		* on creation (Name, Position, Rotation, Scale, Active, WorldMatrix,
		* Bounds), which the node holds as dedicated members or rebuilds
		* each frame, and the velocities Rigidbody attaches with itself
		* (LinearVelocity, AngularVelocity), which come back whenever the
		* Rigidbody is added.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* id が、ノードが取得するのではなく、エンジンが自分で付けるコンポーネント
		* のどれかかどうかを返す。全ての actor に生成時に付くもの（Name、
		* Position、Rotation、Scale、Active、WorldMatrix、Bounds）は、ノードが
		* 専用のメンバーとして持つか、毎フレーム作り直される。Rigidbody が自分と
		* 一緒に付ける速度（LinearVelocity、AngularVelocity）は、Rigidbody を
		* 追加すれば必ず付き直す。
		*/
		static Bool Builtin(ComponentID id);

		/**
		* [EN]
		* Appends actor to nodes with parentIndex as its parent, then its
		* descendants after it, depth first, and returns actor's own index
		* within nodes. A non-root actor that is a nested prefab instance's
		* root records only the prefab reference, so neither its components
		* nor its descendants are captured.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* actor を、parentIndex を親として nodes へ追加し、その後に子孫を
		* 深さ優先で続け、nodes 内での actor 自身のインデックスを返す。
		* ネストされたプレハブインスタンスのルートである非ルートの actor は、
		* プレハブへの参照だけを記録するので、そのコンポーネントも子孫も
		* 取得しない。
		*/
		static Int Capture(Actor actor, Int parentIndex, DynamicArray<BlueprintNode>& nodes);
	};
}