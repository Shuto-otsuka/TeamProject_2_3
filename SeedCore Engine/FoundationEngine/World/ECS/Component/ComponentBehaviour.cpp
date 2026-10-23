#include <FoundationEngine/World/ECS/Component/ComponentBehaviour.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
	/**
	* [EN]
	* Returns the Actor this component is attached to, or an invalid Actor
	* if the component has not been attached yet.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このコンポーネントがアタッチされている Actor を返す。まだアタッチ
	* されていなければ無効な Actor。
	*/
	Actor ComponentBehaviour::GetActor()const
	{
		/// [EN] Actor's constructor is private, so the handle is built through ActorAccessor, the engine's one way to make one.
		/// [JP] Actor のコンストラクタは private なので、エンジンが Actor を作る唯一の入口である ActorAccessor を通して作る。
		if (!world_)
		{
			return Actor();
		}
		return ActorAccessor::Make(*world_, entity_);
	}

	/**
	* [EN]
	* Returns the World that owns this component's Actor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このコンポーネントの Actor を所有する World を返す。
	*/
	World& ComponentBehaviour::GetWorld()const
	{
		return *world_;
	}

	/**
	* [EN]
	* Returns the name this component's type is registered under, read
	* from its ComponentRecord.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このコンポーネントの型が登録されている名前を返す。ComponentRecord
	* から読む。
	*/
	const String& ComponentBehaviour::GetName()const
	{
		/// [EN] Records are never freed, so the reference stays valid for as long as the caller holds it.
		/// [JP] レコードは解放されないので、返した参照は呼び出し側が持っている間ずっと有効。
		return id_->name_;
	}

	/**
	* [EN]
	* Dispatches the OnAttach event, run once the moment this component
	* is added to its actor, to this component's implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このコンポーネントが actor に追加された瞬間に1回だけ走る OnAttach
	* イベントを、このコンポーネントの実装（存在する場合）へディスパッチ
	* する。
	*/
	void ComponentBehaviour::DispatchAttach()
	{
		/// [EN] An instance not yet attached has no id_, and a type that does not implement the hook leaves it nullptr; either way there is nothing to call.
		/// [JP] まだアタッチされていないインスタンスは id_ を持たず、フックを実装していない型ではそのフックが nullptr。どちらも呼ぶものが無い。
		if (id_ && id_->hooks_ && id_->hooks_->attach_)
		{
			id_->hooks_->attach_(this);
		}
	}

	/**
	* [EN]
	* Dispatches the OnDetach event, run the moment this component is
	* removed from its actor, to this component's implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このコンポーネントが actor から外される瞬間に走る OnDetach イベント
	* を、このコンポーネントの実装（存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchDetach()
	{
		/// [EN] An instance not yet attached has no id_, and a type that does not implement the hook leaves it nullptr; either way there is nothing to call.
		/// [JP] まだアタッチされていないインスタンスは id_ を持たず、フックを実装していない型ではそのフックが nullptr。どちらも呼ぶものが無い。
		if (id_ && id_->hooks_ && id_->hooks_->detach_)
		{
			id_->hooks_->detach_(this);
		}
	}

	/**
	* [EN]
	* Dispatches the OnAwake lifecycle event to this component's
	* implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* OnAwake ライフサイクルイベントを、このコンポーネントの実装
	* （存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchAwake()
	{
		if (id_ && id_->hooks_ && id_->hooks_->awake_)
		{
			id_->hooks_->awake_(this);
		}
	}

	/**
	* [EN]
	* Dispatches the OnStart lifecycle event to this component's
	* implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* OnStart ライフサイクルイベントを、このコンポーネントの実装
	* （存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchStart()
	{
		if (id_ && id_->hooks_ && id_->hooks_->start_)
		{
			id_->hooks_->start_(this);
		}
	}

	/**
	* [EN]
	* Dispatches the OnTick lifecycle event, with the frame's elapsed
	* time, to this component's implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* OnTick ライフサイクルイベントを、フレームの経過時間とともに、
	* このコンポーネントの実装（存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchTick(Float elapsedTime)
	{
		if (id_ && id_->hooks_ && id_->hooks_->tick_)
		{
			id_->hooks_->tick_(this, elapsedTime);
		}
	}

	/**
	* [EN]
	* Dispatches the OnFixedTick lifecycle event, with the fixed step's
	* length, to this component's implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* OnFixedTick ライフサイクルイベントを、固定ステップの長さとともに、
	* このコンポーネントの実装（存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchFixedTick(Float elapsedTime)
	{
		if (id_ && id_->hooks_ && id_->hooks_->fixedTick_)
		{
			id_->hooks_->fixedTick_(this, elapsedTime);
		}
	}

	/**
	* [EN]
	* Dispatches the OnLateTick lifecycle event, with the frame's
	* elapsed time, to this component's implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* OnLateTick ライフサイクルイベントを、フレームの経過時間とともに、
	* このコンポーネントの実装（存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchLateTick(Float elapsedTime)
	{
		if (id_ && id_->hooks_ && id_->hooks_->lateTick_)
		{
			id_->hooks_->lateTick_(this, elapsedTime);
		}
	}

	/**
	* [EN]
	* Dispatches the OnEditorTick lifecycle event, with the editor's
	* elapsed time, to this component's implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* OnEditorTick ライフサイクルイベントを、エディタの経過時間とともに、
	* このコンポーネントの実装（存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchEditorTick(Float elapsedTime)
	{
		if (id_ && id_->hooks_ && id_->hooks_->editorTick_)
		{
			id_->hooks_->editorTick_(this, elapsedTime);
		}
	}

	/**
	* [EN]
	* Dispatches the OnDestroy lifecycle event to this component's
	* implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* OnDestroy ライフサイクルイベントを、このコンポーネントの実装
	* （存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchDestroy()
	{
		if (id_ && id_->hooks_ && id_->hooks_->destroy_)
		{
			id_->hooks_->destroy_(this);
		}
	}

	/**
	* [EN]
	* Dispatches the inspector-GUI draw call to this component's
	* implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* インスペクタ GUI の描画呼び出しを、このコンポーネントの実装
	* （存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchInspectorGUI()
	{
		if (id_ && id_->hooks_ && id_->hooks_->inspectorGUI_)
		{
			id_->hooks_->inspectorGUI_(this);
		}
	}

	/**
	* [EN]
	* Dispatches a collision-enter event involving other to this
	* component's implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* other との衝突開始イベントを、このコンポーネントの実装
	* （存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchCollisionEnter(const Actor& other)
	{
		if (id_ && id_->hooks_ && id_->hooks_->collisionEnter_)
		{
			id_->hooks_->collisionEnter_(this, other);
		}
	}

	/**
	* [EN]
	* Dispatches a collision-stay event involving other to this
	* component's implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* other との衝突継続イベントを、このコンポーネントの実装
	* （存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchCollisionStay(const Actor& other)
	{
		if (id_ && id_->hooks_ && id_->hooks_->collisionStay_)
		{
			id_->hooks_->collisionStay_(this, other);
		}
	}

	/**
	* [EN]
	* Dispatches a collision-exit event involving other to this
	* component's implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* other との衝突終了イベントを、このコンポーネントの実装
	* （存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchCollisionExit(const Actor& other)
	{
		if (id_ && id_->hooks_ && id_->hooks_->collisionExit_)
		{
			id_->hooks_->collisionExit_(this, other);
		}
	}

	/**
	* [EN]
	* Dispatches a trigger-enter event involving other to this
	* component's implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* other とのトリガー開始イベントを、このコンポーネントの実装
	* （存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchTriggerEnter(const Actor& other)
	{
		if (id_ && id_->hooks_ && id_->hooks_->triggerEnter_)
		{
			id_->hooks_->triggerEnter_(this, other);
		}
	}

	/**
	* [EN]
	* Dispatches a trigger-stay event involving other to this
	* component's implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* other とのトリガー継続イベントを、このコンポーネントの実装
	* （存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchTriggerStay(const Actor& other)
	{
		if (id_ && id_->hooks_ && id_->hooks_->triggerStay_)
		{
			id_->hooks_->triggerStay_(this, other);
		}
	}

	/**
	* [EN]
	* Dispatches a trigger-exit event involving other to this
	* component's implementation, if any.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* other とのトリガー終了イベントを、このコンポーネントの実装
	* （存在する場合）へディスパッチする。
	*/
	void ComponentBehaviour::DispatchTriggerExit(const Actor& other)
	{
		if (id_ && id_->hooks_ && id_->hooks_->triggerExit_)
		{
			id_->hooks_->triggerExit_(this, other);
		}
	}

	/**
	* [EN]
	* Resets the awoken_/started_ flags, allowing Awake/Start to fire
	* again as if the component were freshly attached.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* awoken_/started_ フラグをリセットし、コンポーネントが新たに
	* アタッチされたかのように Awake/Start を再度発火できるようにする。
	*/
	void ComponentBehaviour::ResetLifecycle()
	{
		awoken_ = false;
		started_ = false;
	}
}
