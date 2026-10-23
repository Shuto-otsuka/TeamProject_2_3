#include <FoundationEngine/World/Actor/ActorRegistry.h>

namespace SeedCore
{
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
	ActorRegistry::ActorRegistry(EntityRegistry& entities) :entities_(entities)
	{
		/// No Code
	}

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
	void ActorRegistry::Add(EntityID entityID, Uint32 persistentID)
	{
		if (!entities_.Exists(entityID))
		{
			return;
		}

		if (entityID.index_ >= records_.size())
		{
			records_.resize(entityID.index_ + 1);
		}
		records_[entityID.index_] = ActorRecord{};

		/// [EN] A loaded ID moves the counter past it, so IDs handed out later never collide with saved ones; the counter is therefore always past every ID in use, which also makes it a safe replacement for a taken one.
		/// [JP] 読み込んだ ID があればカウンタをその先へ進め、後で払い出す ID が保存済みの ID とぶつからないようにする。そのためカウンタは常に使用中の全 ID より先にあり、既に使われている ID の代わりとしても安全に使える。
		if (persistentID == 0 || (persistentID < persistentIndex_.size() && persistentIndex_[persistentID] != EntityID{}))
		{
			persistentID = nextPersistentID_++;
		}
		else
		{
			nextPersistentID_ = Max(nextPersistentID_, persistentID + 1);
		}

		if (persistentID >= persistentIndex_.size())
		{
			persistentIndex_.resize(persistentID + 1);
		}
		persistentIndex_[persistentID] = entityID;
		records_[entityID.index_].persistentID_ = persistentID;

		/// [EN] The first root holds the last root in previousSibling_, so appending goes straight after it.
		/// [JP] 先頭のルートは previousSibling_ に末尾のルートを持つので、その直後へそのまま追加できる。
		EntityID last = (root_ != EntityID{}) ? records_[root_.index_].previousSibling_ : EntityID{};
		Link(entityID, EntityID{}, last);
	}

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
	void ActorRegistry::Remove(EntityID entityID)
	{
		ActorRecord* record = Record(entityID);
		if (!record)
		{
			return;
		}

		SC_ASSERT(record->child_ == EntityID{}, "子が残っている actor を Remove しようとしました。子を先に取り除いてください。");

		Unlink(entityID);
		persistentIndex_[record->persistentID_] = EntityID{};

		/// [EN] Resetting the record also frees extraTags_.
		/// [JP] レコードを初期化すると extraTags_ も解放される。
		*record = ActorRecord{};
	}

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
	void ActorRegistry::Attach(EntityID child, EntityID parent)
	{
		if (!Record(child) || (parent != EntityID{} && !Record(parent)))
		{
			return;
		}

		/// [EN] Walking up from parent reaches child only when parent is child or below it, which would make a cycle.
		/// [JP] parent から上へたどって child に当たるのは、parent が child 自身かその下にいるときだけで、そうなると輪ができる。
		for (EntityID ancestor = parent;ancestor != EntityID{};ancestor = records_[ancestor.index_].parent_)
		{
			if (ancestor == child)
			{
				return;
			}
		}

		Unlink(child);

		EntityID head = (parent != EntityID{}) ? records_[parent.index_].child_ : root_;
		EntityID last = (head != EntityID{}) ? records_[head.index_].previousSibling_ : EntityID{};
		Link(child, parent, last);
	}

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
	void ActorRegistry::Move(EntityID child, EntityID after)
	{
		ActorRecord* record = Record(child);
		if (!record || after == child)
		{
			return;
		}

		/// [EN] Only siblings can be reordered against each other.
		/// [JP] 並べ替えられるのは兄弟どうしだけ。
		if (after != EntityID{})
		{
			ActorRecord* afterRecord = Record(after);
			if (!afterRecord || afterRecord->parent_ != record->parent_)
			{
				return;
			}
		}

		Unlink(child);
		Link(child, record->parent_, after);
	}

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
	EntityID ActorRegistry::Root()const
	{
		return root_;
	}

	/**
	* [EN]
	* Returns the actor after entityID in depth-first order: its first
	* child, otherwise the next sibling of it or of its nearest ancestor
	* that has one. An invalid EntityID after the last actor, or if
	* entityID is stale.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 深さ優先の順で entityID の次の actor を返す: 先頭の子、無ければ自身か、
	* 次の兄弟を持つ一番近い祖先の、次の兄弟。最後の actor の次、または
	* entityID が古ければ無効な EntityID。
	*/
	EntityID ActorRegistry::Next(EntityID entityID)const
	{
		const ActorRecord* record = Record(entityID);
		if (!record)
		{
			return EntityID{};
		}

		if (record->child_ != EntityID{})
		{
			return record->child_;
		}

		/// [EN] With no child, climb from entityID toward the root; the first one met that has a next sibling leads to it. The scene roots are siblings too, so climbing past the last root ends the walk.
		/// [JP] 子が無ければ entityID からルートへ向かって上がり、最初に見つかった次の兄弟を持つものから、その兄弟へ進む。シーンのルート同士も兄弟なので、最後のルートを越えて上がれば走査が終わる。
		for (EntityID current = entityID;current != EntityID{};current = records_[current.index_].parent_)
		{
			EntityID sibling = records_[current.index_].nextSibling_;
			if (sibling != EntityID{})
			{
				return sibling;
			}
		}
		return EntityID{};
	}

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
	EntityID ActorRegistry::Find(Uint32 persistentID)const
	{
		if (persistentID >= persistentIndex_.size())
		{
			return EntityID{};
		}
		return persistentIndex_[persistentID];
	}

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
	void ActorRegistry::Link(EntityID child, EntityID parent, EntityID after)
	{
		ActorRecord& record = records_[child.index_];
		record.parent_ = parent;

		EntityID& head = (parent != EntityID{}) ? records_[parent.index_].child_ : root_;

		/// [EN] An empty list: child is both first and last, so it links to itself as the last.
		/// [JP] 空のリスト: child が先頭かつ末尾なので、末尾として自分自身につなぐ。
		if (head == EntityID{})
		{
			head = child;
			record.previousSibling_ = child;
			record.nextSibling_ = EntityID{};
			return;
		}

		ActorRecord& first = records_[head.index_];

		/// [EN] Into the front: child takes over the link to the last, and the old first now points back to child.
		/// [JP] 先頭へ入れる: child が末尾へのつなぎを引き継ぎ、元の先頭は child を前として指す。
		if (after == EntityID{})
		{
			record.previousSibling_ = first.previousSibling_;
			record.nextSibling_ = head;
			first.previousSibling_ = child;
			head = child;
			return;
		}

		ActorRecord& afterRecord = records_[after.index_];
		record.previousSibling_ = after;
		record.nextSibling_ = afterRecord.nextSibling_;

		/// [EN] Into the end: the first one's link to the last moves to child.
		/// [JP] 末尾へ入れる: 先頭が持つ末尾へのつなぎを child へ付け替える。
		if (afterRecord.nextSibling_ != EntityID{})
		{
			records_[afterRecord.nextSibling_.index_].previousSibling_ = child;
		}
		else
		{
			first.previousSibling_ = child;
		}
		afterRecord.nextSibling_ = child;
	}

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
	void ActorRegistry::Unlink(EntityID child)
	{
		ActorRecord& record = records_[child.index_];
		EntityID& head = (record.parent_ != EntityID{}) ? records_[record.parent_.index_].child_ : root_;

		if (head == child)
		{
			/// [EN] Removing the first: the next one becomes first and inherits the link to the last.
			/// [JP] 先頭を外す場合: 次の子が先頭になり、末尾へのつなぎを引き継ぐ。
			if (record.nextSibling_ != EntityID{})
			{
				records_[record.nextSibling_.index_].previousSibling_ = record.previousSibling_;
			}
			head = record.nextSibling_;
		}
		else
		{
			records_[record.previousSibling_.index_].nextSibling_ = record.nextSibling_;

			/// [EN] Removing the last: the first one's link to the last moves to the new last.
			/// [JP] 末尾を外す場合: 先頭が持つ末尾へのつなぎを、新しい末尾へ付け替える。
			if (record.nextSibling_ != EntityID{})
			{
				records_[record.nextSibling_.index_].previousSibling_ = record.previousSibling_;
			}
			else
			{
				records_[head.index_].previousSibling_ = record.previousSibling_;
			}
		}

		record.nextSibling_ = EntityID{};
		record.previousSibling_ = EntityID{};
	}
}