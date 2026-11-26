#pragma once
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/Actor/ActorRegistry.h>

#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>

#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/ECS/Entity/EntityRegistry.h>

#include <FoundationEngine/World/ECS/Archetype/ArchetypeStorage.h>
#include <FoundationEngine/World/ECS/SparseSet/SparseSetStorage.h>

#include <FoundationEngine/World/Service.h>

namespace SeedCore
{
	class SEEDCORE_API World
	{
	private:
		friend class WorldAccessor;

	public:
		~World();

	private:
		Actor CreateActor(String name, Uint32 persistentID);

	public:
		Actor CreateActor(String name);

		void DestroyActor(Actor actor);

		Actor GetActor(const String& name);

		Actor GetActor(EntityID entityID);

		Actor GetActor(Uint32 persistentID);

		DynamicArray<Actor> Actors();

	public:
		void* AddComponent(Entity entity, ComponentID id);

		template<typename T>
		T* AddComponent(Entity entity)
		{
			ComponentID id = ComponentRegistry::GetComponentID<T>();
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				return static_cast<T*>(archetypes_.Add(entity.GetID(), id));
			}
			else
			{
				return static_cast<T*>(sparseSets_.Add(entity.GetID(), id));
			}
		}

		template<typename T>
			requires(std::is_copy_assignable_v<T>)
		T* AddComponent(Entity entity, const T& value)
		{
			T* component = AddComponent<T>(entity);
			if (component)
			{
				*component = value;
			}
			return component;
		}

		void* GetComponent(Entity entity, ComponentID id)const;

		template<typename T>
		T* GetComponent(Entity entity)const
		{
			ComponentID id = ComponentRegistry::GetComponentID<T>();
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				return static_cast<T*>(archetypes_.Data(entity.GetID(), id));
			}
			else
			{
				return static_cast<T*>(sparseSets_.Data(entity.GetID(), id));
			}
		}

		Bool HasComponent(Entity entity, ComponentID id)const;

		template<typename T>
		Bool HasComponent(Entity entity)const
		{
			ComponentID id = ComponentRegistry::GetComponentID<T>();
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				return archetypes_.Has(entity.GetID(), id);
			}
			else
			{
				return sparseSets_.Has(entity.GetID(), id);
			}
		}

		void RemoveComponent(Entity entity, ComponentID id);

		template<typename T>
		void RemoveComponent(Entity entity)
		{
			ComponentID id = ComponentRegistry::GetComponentID<T>();
			if constexpr (ComponentTraits<T>::storage == ComponentStorage::Archetype)
			{
				archetypes_.Remove(entity.GetID(), id);
			}
			else
			{
				sparseSets_.Remove(entity.GetID(), id);
			}
		}

	public:
		template<typename T,typename... Arguments>
		T* AddService(Arguments&&... arguments)
		{
			return service_.Add<T>(std::forward<Arguments>(arguments)...);
		}

		template<typename T>
		T* GetService()const
		{
			return service_.Get<T>();
		}

		template<typename T>
		void RemoveService()
		{
			service_.Remove<T>();
		}

	private:
		EntityRegistry entities_;

		ActorRegistry actors_ = ActorRegistry(entities_);

		ArchetypeStorage archetypes_ = ArchetypeStorage(entities_);

		SparseSetStorage sparseSets_ = SparseSetStorage(entities_);

		Service service_;
	};
}