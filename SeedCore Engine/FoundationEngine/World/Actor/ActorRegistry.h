#pragma once
#include <FoundationEngine/World/ECS/Entity/EntityRegistry.h>

namespace SeedCore
{
	/**
	* [EN]
	* Everything recorded about one actor besides its components: its place
	* in the hierarchy, its tags and layer, and its hidden persistent ID.
	* Whether it is active lives in its Active component.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つの actor について、コンポーネント以外に記録する全て。階層での位置、
	* タグとレイヤー、外からは見せない永続 ID。アクティブかどうかは Active
	* コンポーネントが持つ。
	*/
	struct ActorRecord
	{
		/// [EN] This actor's parent; an invalid EntityID for a scene root.
		/// [JP] この actor の親。シーンのルートなら無効な EntityID。
		EntityID parent_;

		/// [EN] This actor's first child; an invalid EntityID if it has none.
		/// [JP] この actor の先頭の子。子が無ければ無効な EntityID。
		EntityID child_;

		/// [EN] The next child of the same parent; an invalid EntityID for the last child.
		/// [JP] 同じ親の次の子。末尾の子では無効な EntityID。
		EntityID nextSibling_;

		/// [EN] The previous child of the same parent; the first child holds the last child here, so appending needs no walk.
		/// [JP] 同じ親の前の子。先頭の子はここに末尾の子を持つので、末尾への追加でたどる必要が無い。
		EntityID previousSibling_;

		/// [EN] Membership of the first 64 tags, indexed by TagRegistry index.
		/// [JP] 先頭 64 個のタグの所属。TagRegistry の番号で引く。
		Uint64 tags_ = 0;

		/// [EN] Membership of tags beyond the first 64, indexed by TagRegistry index minus 64; created only when such a tag is added.
		/// [JP] 64 個目より後のタグの所属。TagRegistry の番号から 64 を引いて引く。そのようなタグを付けたときだけ作る。
		ResourcePtr<Bitset> extraTags_;

		/// [EN] This actor's layer, a LayerRegistry slot index.
		/// [JP] この actor のレイヤー。LayerRegistry のスロット番号。
		Uint32 layer_ = 0;

		/// [EN] World-wide unique identifier round-tripped through save and load; never exposed through Actor. 0 = unassigned.
		/// [JP] セーブとロードを往復する、World 全体で一意な識別子。Actor からは見せない。0 は未割り当て。
		Uint32 persistentID_ = 0;
	};

	/**
	* [EN]
	* One world's actors: one ActorRecord per entity slot, indexed by
	* EntityID::index_, with the hierarchy kept as circular sibling lists
	* (the scene roots form one list starting at the root) and a dense
	* array from persistent ID to actor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 1つのワールドの actor 群。エンティティのスロットごとに ActorRecord を
	* 1つ持ち（EntityID::index_ で引く）、階層は輪になった兄弟リストで持つ
	* （シーンのルート群はルートから始まる1本のリスト）。永続 ID から actor
	* への密配列も持つ。
	*/
	class SEEDCORE_API ActorRegistry :public NonTransferable
	{
	public:
		/**
		* [EN]
		* Constructs an empty registry whose records follow the slots of
		* entities.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レコードが entities のスロットに従う、空のレジストリを構築する。
		*/
		ActorRegistry(EntityRegistry& entities);

		/**
		* [EN]
		* Starts the record of entityID and appends it as the last scene root.
		* A persistentID of 0, or one another actor already holds (the same
		* prefab instantiated twice), hands out a new ID; any other value
		* (from a loaded scene) is registered as is.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID のレコードを始め、シーンのルートの末尾につなぐ。persistentID
		* が 0 か、既に他の actor が持っている値（同じプレハブを2回インスタンス
		* 化した場合など）なら新しい ID を払い出し、それ以外（読み込んだシーン
		* の値）ならそのまま登録する。
		*/
		void Add(EntityID entityID, Uint32 persistentID);

		/**
		* [EN]
		* Unlinks entityID from the hierarchy and releases its persistent ID.
		* Its children must have been removed first.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID を階層から外し、永続 ID を手放す。子は先に取り除いて
		* おかなければならない。
		*/
		void Remove(EntityID entityID);

		/**
		* [EN]
		* Makes child the last child of parent, or the last scene root if
		* parent is invalid. Does nothing if parent is child itself or one of
		* its descendants.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* child を parent の末尾の子にする。parent が無効ならシーンのルートの
		* 末尾にする。parent が child 自身かその子孫なら何もしない。
		*/
		void Attach(EntityID child, EntityID parent);

		/**
		* [EN]
		* Moves child right after after among the children of their shared
		* parent, or to the front if after is invalid.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 同じ親の子の中で、child を after の直後へ移す。after が無効なら
		* 先頭へ移す。
		*/
		void Move(EntityID child, EntityID after);

		/**
		* [EN]
		* Returns the first scene root; the others follow through
		* nextSibling_. An invalid EntityID if there are no actors.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 先頭のシーンのルートを返す。残りは nextSibling_ でたどる。actor が
		* 居なければ無効な EntityID。
		*/
		EntityID Root()const;

		/**
		* [EN]
		* Returns the actor after entityID in depth-first order: its first
		* child, otherwise the next sibling of it or of its nearest ancestor
		* that has one. Starting from Root() and following Next visits every
		* actor once, each parent before its children. An invalid EntityID
		* after the last actor, or if entityID is stale.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 深さ優先の順で entityID の次の actor を返す: 先頭の子、無ければ自身か、
		* 次の兄弟を持つ一番近い祖先の、次の兄弟。Root() から Next をたどれば、
		* 全ての actor を1回ずつ、親を子より先に回れる。最後の actor の次、
		* または entityID が古ければ無効な EntityID。
		*/
		EntityID Next(EntityID entityID)const;

		/**
		* [EN]
		* Returns the actor holding persistentID, or an invalid EntityID if
		* none does.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* persistentID を持つ actor を返す。誰も持っていなければ無効な
		* EntityID。
		*/
		EntityID Find(Uint32 persistentID)const;

		/**
		* [EN]
		* Returns entityID's record, or nullptr if entityID is stale; the
		* record is const when the registry is.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* entityID のレコードを返す。entityID が古ければ nullptr。registry が
		* const ならレコードも const になる。
		*/
		template<typename Self>
		auto Record(this Self& self, EntityID entityID) -> decltype(&self.records_[0])
		{
			if (!self.entities_.Exists(entityID) || entityID.index_ >= self.records_.size())
			{
				return nullptr;
			}
			return &self.records_[entityID.index_];
		}

	private:
		/**
		* [EN]
		* Puts child into the sibling list of parent (the scene roots if parent
		* is invalid) right after after, or at the front if after is invalid,
		* and sets its parent_.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* child を parent の兄弟リスト（parent が無効ならシーンのルート群）の、
		* after の直後へ入れる。after が無効なら先頭へ入れる。parent_ も設定する。
		*/
		void Link(EntityID child, EntityID parent, EntityID after);

		/**
		* [EN]
		* Takes child out of its sibling list (its parent's children, or the
		* scene roots), keeping parent_ as it is.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* child を兄弟リスト（親の子の並び、またはシーンのルート群）から外す。
		* parent_ はそのまま残す。
		*/
		void Unlink(EntityID child);

	private:
		/// [EN] Entity slots the records follow; owned by the world.
		/// [JP] レコードが従うエンティティのスロット。ワールドが所有する。
		EntityRegistry& entities_;

		/// [EN] One record per entity slot, indexed by EntityID::index_.
		/// [JP] エンティティのスロットごとのレコード。EntityID::index_ で引く。
		DynamicArray<ActorRecord> records_;

		/// [EN] The first scene root; the roots are linked through their sibling fields like the children of one parent.
		/// [JP] 先頭のシーンのルート。ルート群は、1つの親の子と同じく兄弟のフィールドでつながる。
		EntityID root_;

		/// [EN] Maps a persistent ID to its actor, indexed by the ID itself; an invalid EntityID where no actor holds that ID.
		/// [JP] 永続 ID を actor へ対応付ける。ID そのもので引く。その ID を持つ actor が居ない所は無効な EntityID。
		DynamicArray<EntityID> persistentIndex_;

		/// [EN] Next persistent ID handed out by Add; 0 is reserved as unassigned, so this starts at 1.
		/// [JP] Add が次に払い出す永続 ID。0 は未割り当てとして予約されているので 1 から始まる。
		Uint32 nextPersistentID_ = 1;
	};
}