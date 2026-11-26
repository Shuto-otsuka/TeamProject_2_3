#include <FoundationEngine/World/ECS/Component/ComponentBehaviour.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
	Actor ComponentBehaviour::GetActor()const
	{
		if (!world_)
		{
			return Actor();
		}
		return ActorAccessor::Make(*world_, entity_);
	}

	World& ComponentBehaviour::GetWorld()const
	{
		return *world_;
	}

	const String& ComponentBehaviour::GetName()const
	{
		return id_->name_;
	}

	void ComponentBehaviour::DispatchAttach()
	{
		if (id_ && id_->hooks_ && id_->hooks_->attach_)
		{
			id_->hooks_->attach_(this);
		}
	}

	void ComponentBehaviour::DispatchDetach()
	{
		if (id_ && id_->hooks_ && id_->hooks_->detach_)
		{
			id_->hooks_->detach_(this);
		}
	}

	void ComponentBehaviour::DispatchAwake()
	{
		if (id_ && id_->hooks_ && id_->hooks_->awake_)
		{
			id_->hooks_->awake_(this);
		}
	}

	void ComponentBehaviour::DispatchStart()
	{
		if (id_ && id_->hooks_ && id_->hooks_->start_)
		{
			id_->hooks_->start_(this);
		}
	}

	void ComponentBehaviour::DispatchTick(Float elapsedTime)
	{
		if (id_ && id_->hooks_ && id_->hooks_->tick_)
		{
			id_->hooks_->tick_(this, elapsedTime);
		}
	}

	void ComponentBehaviour::DispatchFixedTick(Float elapsedTime)
	{
		if (id_ && id_->hooks_ && id_->hooks_->fixedTick_)
		{
			id_->hooks_->fixedTick_(this, elapsedTime);
		}
	}

	void ComponentBehaviour::DispatchLateTick(Float elapsedTime)
	{
		if (id_ && id_->hooks_ && id_->hooks_->lateTick_)
		{
			id_->hooks_->lateTick_(this, elapsedTime);
		}
	}

	void ComponentBehaviour::DispatchEditorTick(Float elapsedTime)
	{
		if (id_ && id_->hooks_ && id_->hooks_->editorTick_)
		{
			id_->hooks_->editorTick_(this, elapsedTime);
		}
	}

	void ComponentBehaviour::DispatchDestroy()
	{
		if (id_ && id_->hooks_ && id_->hooks_->destroy_)
		{
			id_->hooks_->destroy_(this);
		}
	}

	void ComponentBehaviour::DispatchInspectorGUI()
	{
		if (id_ && id_->hooks_ && id_->hooks_->inspectorGUI_)
		{
			id_->hooks_->inspectorGUI_(this);
		}
	}

	void ComponentBehaviour::DispatchCollisionEnter(const Actor& other)
	{
		if (id_ && id_->hooks_ && id_->hooks_->collisionEnter_)
		{
			id_->hooks_->collisionEnter_(this, other);
		}
	}

	void ComponentBehaviour::DispatchCollisionStay(const Actor& other)
	{
		if (id_ && id_->hooks_ && id_->hooks_->collisionStay_)
		{
			id_->hooks_->collisionStay_(this, other);
		}
	}

	void ComponentBehaviour::DispatchCollisionExit(const Actor& other)
	{
		if (id_ && id_->hooks_ && id_->hooks_->collisionExit_)
		{
			id_->hooks_->collisionExit_(this, other);
		}
	}

	void ComponentBehaviour::DispatchTriggerEnter(const Actor& other)
	{
		if (id_ && id_->hooks_ && id_->hooks_->triggerEnter_)
		{
			id_->hooks_->triggerEnter_(this, other);
		}
	}

	void ComponentBehaviour::DispatchTriggerStay(const Actor& other)
	{
		if (id_ && id_->hooks_ && id_->hooks_->triggerStay_)
		{
			id_->hooks_->triggerStay_(this, other);
		}
	}

	void ComponentBehaviour::DispatchTriggerExit(const Actor& other)
	{
		if (id_ && id_->hooks_ && id_->hooks_->triggerExit_)
		{
			id_->hooks_->triggerExit_(this, other);
		}
	}

	void ComponentBehaviour::DispatchRevision()
	{
		if (id_ && id_->hooks_ && id_->hooks_->revision_)
		{
			id_->hooks_->revision_(this);
		}
	}

	void ComponentBehaviour::ResetLifecycle()
	{
		awoken_ = false;
		started_ = false;
	}
}
