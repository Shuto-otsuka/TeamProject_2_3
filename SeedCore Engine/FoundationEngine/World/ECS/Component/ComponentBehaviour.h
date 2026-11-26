#pragma once
#include <FoundationEngine/World/ECS/Component/Attachable.h>
#include <FoundationEngine/World/ECS/Component/Detachable.h>
#include <FoundationEngine/World/ECS/Component/Awakeable.h>
#include <FoundationEngine/World/ECS/Component/Startable.h>
#include <FoundationEngine/World/ECS/Component/Tickable.h>
#include <FoundationEngine/World/ECS/Component/FixedTickable.h>
#include <FoundationEngine/World/ECS/Component/LateTickable.h>
#include <FoundationEngine/World/ECS/Component/Editable.h>
#include <FoundationEngine/World/ECS/Component/Destroyable.h>
#include <FoundationEngine/World/ECS/Component/InspectorDrawable.h>
#include <FoundationEngine/World/ECS/Component/Collisionable.h>
#include <FoundationEngine/World/ECS/Component/Triggerable.h>
#include <FoundationEngine/World/ECS/Component/Revisionable.h>
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	class Actor;
	class World;

	class SEEDCORE_API ComponentBehaviour
	{
	private:
		friend class Actor;
		friend class CsharpBehaviour;
		friend class SystemScheduler;
		friend class World;
		friend class WorldSnapshot;

	public:
		using is_component_behaviour_tag = void;

		virtual ~ComponentBehaviour() = default;

		Actor GetActor()const;

		World& GetWorld()const;

		const String& GetName()const;

		void DispatchAttach();

		void DispatchDetach();

		void DispatchAwake();

		void DispatchStart();

		void DispatchTick(Float elapsedTime);

		void DispatchFixedTick(Float elapsedTime);

		void DispatchLateTick(Float elapsedTime);

		void DispatchEditorTick(Float elapsedTime);

		void DispatchDestroy();

		void DispatchInspectorGUI();

		void DispatchCollisionEnter(const Actor& other);

		void DispatchCollisionStay(const Actor& other);

		void DispatchCollisionExit(const Actor& other);

		void DispatchTriggerEnter(const Actor& other);

		void DispatchTriggerStay(const Actor& other);

		void DispatchTriggerExit(const Actor& other);

		void DispatchRevision();

	private:
		void ResetLifecycle();

	private:
		World* world_ = nullptr;

		Entity entity_;

		ComponentID id_ = nullptr;

		Bool awoken_ = false;

		Bool started_ = false;
	};
}
