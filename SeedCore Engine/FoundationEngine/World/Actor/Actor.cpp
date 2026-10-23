#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Tag/TagRegistry.h>
#include <FoundationEngine/World/Layer/LayerRegistry.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/World/ECS/Component/WorldMatrix.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs a handle to the actor identified by entity within world.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world 内で entity が識別する actor へのハンドルを構築する。
	*/
	Actor::Actor(World& world, Entity entity) : world_(&world), entity_(entity)
	{
		/// No Code
	}

	/**
	* [EN]
	* Returns whether this handle refers to a live actor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このハンドルが生存中の actor を指しているかどうかを返す。
	*/
	Actor::operator Bool()const
	{
		return world_ != nullptr && WorldAccessor::Entities(*world_).Exists(entity_.GetID());
	}

	/**
	* [EN]
	* Returns whether this and other are handles to the same actor: they
	* refer to the same World and the same Entity.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* this と other が同じ actor へのハンドルかどうかを返す: 同じ World と
	* 同じ Entity を指しているか。
	*/
	Bool Actor::operator==(const Actor& other)const
	{
		return world_ == other.world_ && entity_ == other.entity_;
	}

	/**
	* [EN]
	* Type-erased overload of AddComponent: adds a default-constructed
	* instance of the component registered under id and returns its
	* address, or returns the existing one if this actor already has it.
	* A newly added instance of a type with a hook table is wired to this
	* actor and handed to the type's attach_.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* AddComponent の型消去版: id に登録されているコンポーネントの
	* デフォルト構築されたインスタンスを追加し、そのアドレスを返す。この
	* actor が既に持っていれば、既存のものを返す。フックの表を持つ型の、
	* 新しく追加したインスタンスは、この actor へ結び付けてから型の
	* attach_ に渡す。
	*/
	void* Actor::AddComponent(ComponentID id)
	{
		if (!world_ || !id)
		{
			return nullptr;
		}

		/// [EN] An existing instance is already wired, so it is returned as is rather than attached a second time.
		/// [JP] 既にあるインスタンスは配線済みなので、二度目の attach をせずにそのまま返す。
		if (void* existing = world_->GetComponent(entity_, id))
		{
			return existing;
		}

		void* data = world_->AddComponent(entity_, id);
		if (!data || !id->hooks_ || !id->hooks_->upcast_)
		{
			return data;
		}

		/// [EN] A behaviour learns its world, entity and registration here, now that it exists in storage, and only then is it attached.
		/// [JP] ビヘイビアはストレージ上に存在するようになったここで、World・エンティティ・登録を受け取り、そのあとで attach される。
		ComponentBehaviour* behaviour = id->hooks_->upcast_(data);
		behaviour->world_ = world_;
		behaviour->entity_ = entity_;
		behaviour->id_ = id;
		if (id->hooks_->attach_)
		{
			id->hooks_->attach_(behaviour);
		}
		return data;
	}

	/**
	* [EN]
	* Type-erased overload of GetComponent: returns the address of this
	* actor's component registered under id, or nullptr if absent.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* GetComponent の型消去版: この actor の、id に登録されている
	* コンポーネントのアドレスを返す。無ければ nullptr を返す。
	*/
	void* Actor::GetComponent(ComponentID id)const
	{
		if (!world_)
		{
			return nullptr;
		}
		return world_->GetComponent(entity_, id);
	}

	/**
	* [EN]
	* Type-erased overload of HasComponent: returns whether this actor
	* currently has the component registered under id.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* HasComponent の型消去版: この actor が現在、id に登録されている
	* コンポーネントを持っているかどうかを返す。
	*/
	Bool Actor::HasComponent(ComponentID id)const
	{
		return world_ != nullptr && world_->HasComponent(entity_, id);
	}

	/**
	* [EN]
	* Type-erased overload of RemoveComponent: removes this actor's
	* component registered under id, invoking its OnDestroy and then its OnDetach first if it
	* derives from ComponentBehaviour.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* RemoveComponent の型消去版: この actor の、id に登録されている
	* コンポーネントを削除する。ComponentBehaviour から派生していれば、
	* 先に OnDestroy、続いて OnDetach を呼び出す。
	*/
	void Actor::RemoveComponent(ComponentID id)
	{
		if (!world_ || !id)
		{
			return;
		}

		/// [EN] OnDestroy and then OnDetach run while the component still exists in storage, before it is removed: the reverse of OnAttach then OnAwake on the way in.
		/// [JP] OnDestroy、続いて OnDetach を、コンポーネントがまだストレージ上に存在する、削除の前に呼ぶ。追加のときの OnAttach → OnAwake の逆の順。
		if (id->hooks_ && id->hooks_->upcast_)
		{
			if (void* data = world_->GetComponent(entity_, id))
			{
				ComponentBehaviour* behaviour = id->hooks_->upcast_(data);
				behaviour->DispatchDestroy();
				behaviour->DispatchDetach();
			}
		}

		world_->RemoveComponent(entity_, id);
	}

	/**
	* [EN]
	* Returns the World that owns this actor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この actor を所有する World を返す。
	*/
	World& Actor::GetWorld()const
	{
		return *world_;
	}

	/**
	* [EN]
	* Reparents this actor under parent (or to the scene root if parent is
	* an invalid Actor) as its last child. An inactive new parent makes this
	* actor inactive; moving to the scene root makes it active again. Does
	* nothing if parent is this actor or one of its descendants.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この actor を parent の末尾の子として付け替える（parent が無効な
	* Actor であればシーンのルートへ）。新しい親が非アクティブならこの actor
	* も非アクティブになり、シーンのルートへ移るとアクティブに戻る。parent
	* がこの actor 自身かその子孫なら何もしない。
	*/
	void Actor::Parent(Actor parent)
	{
		if (!world_)
		{
			return;
		}

		ActorRegistry& actors = WorldAccessor::Actors(*world_);
		const ActorRecord* record = actors.Record(entity_.GetID());
		if (!record)
		{
			return;
		}

		EntityID parentID = parent ? parent.entity_.GetID() : EntityID{};
		if (record->parent_ == parentID)
		{
			return;
		}

		/// [EN] Attach refuses a parent that would make a cycle, so the parent is checked again before the active state follows it.
		/// [JP] Attach は輪になる親を拒否するので、アクティブ状態を合わせる前に親をもう一度確かめる。
		actors.Attach(entity_.GetID(), parentID);
		if (record->parent_ != parentID)
		{
			return;
		}

		if (parentID != EntityID{})
		{
			if (!parent.Active())
			{
				Active(false);
			}
		}
		else
		{
			Active(true);
		}
	}

	/**
	* [EN]
	* Returns this actor's current parent, or an invalid Actor if it has
	* none.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この actor の現在の親を返す。親が無ければ無効な Actor を返す。
	*/
	Actor Actor::Parent()const
	{
		if (!world_)
		{
			return Actor();
		}

		const ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		if (!record)
		{
			return Actor();
		}
		return WorldAccessor::Make(*world_, record->parent_);
	}

	/**
	* [EN]
	* Returns this actor's direct children, in their current order.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この actor の直接の子一覧を、現在の順序で返す。
	*/
	DynamicArray<Actor> Actor::Children()const
	{
		DynamicArray<Actor> result;
		if (!world_)
		{
			return result;
		}

		ActorRegistry& actors = WorldAccessor::Actors(*world_);
		const ActorRecord* record = actors.Record(entity_.GetID());
		if (!record)
		{
			return result;
		}

		/// [EN] The children are linked from the first child through nextSibling_, ending at an invalid EntityID.
		/// [JP] 子は先頭の子から nextSibling_ でつながっていて、無効な EntityID で終わる。
		for (EntityID child = record->child_; child != EntityID{}; child = actors.Record(child)->nextSibling_)
		{
			result.push_back(Actor(*world_, Entity(Handle<Entity>{ child.index_, child.generation_ })));
		}
		return result;
	}

	/**
	* [EN]
	* Moves this actor right after after among its siblings (the children
	* of its parent, or the scene roots), or to the front if after is
	* invalid. Does nothing if after is not one of its siblings.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 兄弟（親の子の並び、またはシーンのルート群）の中で、この actor を
	* after の直後へ移す。after が無効なら先頭へ移す。after が兄弟で
	* なければ何もしない。
	*/
	void Actor::Move(Actor after)
	{
		if (!world_)
		{
			return;
		}

		EntityID afterID = after ? after.entity_.GetID() : EntityID{};
		WorldAccessor::Actors(*world_).Move(entity_.GetID(), afterID);
	}

	/**
	* [EN]
	* Returns this actor's world-space transform matrix, read from its
	* WorldMatrix component.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この actor のワールド空間変換行列を、WorldMatrix コンポーネントから
	* 読んで返す。
	*/
	const Matrix& Actor::WorldMatrix()const
	{
		/// [EN] An invalid actor has no component, so it reads as the identity matrix.
		/// [JP] 無効な actor はコンポーネントを持たないので、単位行列として読む。
		static const Matrix identity = Matrix::Identity;

		const SeedCore::WorldMatrix* component = GetComponent<SeedCore::WorldMatrix>();
		if (!component)
		{
			return identity;
		}
		return component->matrix_;
	}

	/**
	* [EN]
	* Sets this actor's world-space transform matrix in its WorldMatrix
	* component.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この actor のワールド空間変換行列を、WorldMatrix コンポーネントに
	* 設定する。
	*/
	void Actor::WorldMatrix(const Matrix& matrix)
	{
		SeedCore::WorldMatrix* component = GetComponent<SeedCore::WorldMatrix>();
		if (component)
		{
			component->matrix_ = matrix;
		}
	}

	/**
	* [EN]
	* Returns whether this actor is currently active, read from its Active
	* component.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この actor が現在アクティブかどうかを、Active コンポーネントから
	* 読んで返す。
	*/
	Bool Actor::Active()const
	{
		const SeedCore::Active* component = GetComponent<SeedCore::Active>();
		return component != nullptr && component->active_;
	}

	/**
	* [EN]
	* Sets this actor's active state in its Active component, and
	* propagates the same state to every descendant.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この actor のアクティブ状態を Active コンポーネントに設定し、同じ
	* 状態を全ての子孫へ伝播する。
	*/
	void Actor::Active(Bool active)
	{
		SeedCore::Active* component = GetComponent<SeedCore::Active>();
		if (!component)
		{
			return;
		}
		component->active_ = active;

		/// [EN] A child cannot be active while its parent is inactive, so every descendant follows; this only writes components and leaves the hierarchy as it is.
		/// [JP] 親が非アクティブの間は子もアクティブになれないので、全ての子孫が従う。書くのはコンポーネントだけで、階層には触れない。
		for (Actor child : Children())
		{
			child.Active(active);
		}
	}

	/**
	* [EN]
	* Adds tag to this actor's tag set, registering it in TagRegistry if
	* it's new. An empty tag is ignored.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* tag をこの actor のタグ集合へ追加する。新規のタグであれば
	* TagRegistry へ登録する。空の tag は無視する。
	*/
	void Actor::AddTag(String tag)
	{
		if (!world_)
		{
			return;
		}

		ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		Size index = TagRegistry::GetOrCreate(tag);
		if (!record || index == static_cast<Size>(-1))
		{
			return;
		}

		/// [EN] The first 64 tags are bits in the record itself; later ones go to the extra Bitset, created on first need.
		/// [JP] 先頭 64 個のタグはレコード自身のビット。それ以降は追加の Bitset に入れ、初めて要るときに作る。
		if (index < 64)
		{
			record->tags_ |= Uint64{ 1 } << index;
			return;
		}

		if (!record->extraTags_)
		{
			record->extraTags_ = MakePtr<Bitset>();
		}

		Size bit = index - 64;
		if (bit >= record->extraTags_->size())
		{
			record->extraTags_->resize(bit + 1);
		}
		record->extraTags_->set(bit);
	}

	/**
	* [EN]
	* Returns whether this actor currently has tag.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この actor が現在 tag を持っているかどうかを返す。
	*/
	Bool Actor::HasTag(String tag)const
	{
		if (!world_)
		{
			return false;
		}

		const ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		Size index = TagRegistry::Find(tag);
		if (!record || index == static_cast<Size>(-1))
		{
			return false;
		}

		if (index < 64)
		{
			return (record->tags_ & (Uint64{ 1 } << index)) != 0;
		}

		Size bit = index - 64;
		return record->extraTags_ && bit < record->extraTags_->size() && record->extraTags_->test(bit);
	}

	/**
	* [EN]
	* Removes tag from this actor's tag set, if present.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* tag をこの actor のタグ集合から、存在すれば削除する。
	*/
	void Actor::RemoveTag(String tag)
	{
		if (!world_)
		{
			return;
		}

		ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		Size index = TagRegistry::Find(tag);
		if (!record || index == static_cast<Size>(-1))
		{
			return;
		}

		if (index < 64)
		{
			record->tags_ &= ~(Uint64{ 1 } << index);
			return;
		}

		Size bit = index - 64;
		if (record->extraTags_ && bit < record->extraTags_->size())
		{
			record->extraTags_->reset(bit);
		}
	}

	/**
	* [EN]
	* Sets this actor's layer to the LayerRegistry slot at index (clamped
	* to a valid slot if out of range).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この actor のレイヤーを、index の LayerRegistry スロットに設定する
	* （範囲外なら有効なスロットへクランプする）。
	*/
	void Actor::Layer(Size index)
	{
		if (!world_)
		{
			return;
		}

		ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		if (record)
		{
			record->layer_ = static_cast<Uint32>(Min(index, LayerRegistry::LayerCount - 1));
		}
	}

	/**
	* [EN]
	* Sets this actor's layer by name via LayerRegistry::Find, which gives
	* LayerRegistry::DefaultLayer if name isn't registered.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* LayerRegistry::Find 経由で、名前でこの actor のレイヤーを設定する。
	* name が未登録であれば LayerRegistry::DefaultLayer になる。
	*/
	void Actor::Layer(const String& name)
	{
		if (!world_)
		{
			return;
		}

		ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		if (record)
		{
			record->layer_ = static_cast<Uint32>(LayerRegistry::Find(name));
		}
	}

	/**
	* [EN]
	* Returns this actor's current layer index (a LayerRegistry slot).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この actor の現在のレイヤーインデックス（LayerRegistry の
	* スロット）を返す。
	*/
	Size Actor::Layer()const
	{
		if (!world_)
		{
			return LayerRegistry::DefaultLayer;
		}

		const ActorRecord* record = WorldAccessor::Actors(*world_).Record(entity_.GetID());
		return record ? record->layer_ : LayerRegistry::DefaultLayer;
	}

	/**
	* [EN]
	* Returns this actor's current layer's name, via LayerRegistry::Name.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* LayerRegistry::Name 経由で、この actor の現在のレイヤー名を返す。
	*/
	const String& Actor::LayerName()const
	{
		return LayerRegistry::Name(Layer());
	}

	/**
	* [EN]
	* Type-erased overload of GetService: returns the service of this
	* actor's World whose type's name is type (a ServiceType<T>::name_), or
	* nullptr if there is none.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* GetService の型消去版: この actor の World で、型の名前が type
	* （ServiceType<T>::name_）であるサービスを返す。無ければ nullptr。
	*/
	void* Actor::GetService(const String& type)const
	{
		if (!world_)
		{
			return nullptr;
		}
		return WorldAccessor::Services(*world_).Get(type);
	}
}
