#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	struct ComponentRecord;

	using ComponentID = const ComponentRecord*;

	class ComponentBehaviour;

	class Actor;

	enum class ComponentStorage :Uint8
	{
		SparseSet,

		Archetype,
	};

	struct ComponentMetadata
	{
		Size size_;

		Size alignment_;

		ComponentStorage storage_;

		void (*construct_)(void*);

		void (*destruct_)(void*);

		void (*copy_)(void* dest, const void* source);

		void (*move_)(void* dest, void* source);

		void (*relocate_)(void* dest, void* source);
	};

	struct ComponentHooks
	{
		ComponentBehaviour* (*upcast_)(void* component) = nullptr;

		void (*attach_)(ComponentBehaviour*) = nullptr;

		void (*detach_)(ComponentBehaviour*) = nullptr;

		void (*awake_)(ComponentBehaviour*) = nullptr;

		void (*start_)(ComponentBehaviour*) = nullptr;

		void (*tick_)(ComponentBehaviour*, Float) = nullptr;

		void (*fixedTick_)(ComponentBehaviour*, Float) = nullptr;

		void (*lateTick_)(ComponentBehaviour*, Float) = nullptr;

		void (*editorTick_)(ComponentBehaviour*, Float) = nullptr;

		void (*destroy_)(ComponentBehaviour*) = nullptr;

		void (*inspectorGUI_)(ComponentBehaviour*) = nullptr;

		void (*collisionEnter_)(ComponentBehaviour*, const Actor&) = nullptr;

		void (*collisionStay_)(ComponentBehaviour*, const Actor&) = nullptr;

		void (*collisionExit_)(ComponentBehaviour*, const Actor&) = nullptr;

		void (*triggerEnter_)(ComponentBehaviour*, const Actor&) = nullptr;

		void (*triggerStay_)(ComponentBehaviour*, const Actor&) = nullptr;

		void (*triggerExit_)(ComponentBehaviour*, const Actor&) = nullptr;

		void (*revision_)(ComponentBehaviour*) = nullptr;
	};

	struct ComponentRecord
	{
		String name_;

		String category_;

		Uint32 index_ = 0;

		Uint32 epoch_ = 0;

		const ComponentMetadata* metadata_ = nullptr;

		const ComponentHooks* hooks_ = nullptr;
	};

	ComponentStorage DeclaredComponentStorage(void*) = delete;

	template<typename T>
	concept HasDeclaredComponentStorage = requires(T* type)
	{
		{ DeclaredComponentStorage(type) } -> std::same_as<ComponentStorage>;
	};

	template<typename T>
	struct ComponentTraits
	{
		SC_CONSTEX ComponentStorage storage = []()
			{
				if constexpr (HasDeclaredComponentStorage<T>)
				{
					return DeclaredComponentStorage(static_cast<T*>(nullptr));
				}
				else
				{
					return ComponentStorage::SparseSet;
				}
			}();
	};

	template<typename T>
	class Component
	{
	public:
		SC_CONSTEX ComponentStorage Storage()
		{
			return ComponentTraits<T>::storage;
		}

		SC_CONSTEX ComponentMetadata Metadata(ComponentStorage storage)
		{
			return {
				sizeof(T),
				alignof(T),
				storage,
				[](void* ptr) 
				{
					new(ptr) T();
				},
				[](void* ptr) 
				{
					static_cast<T*>(ptr)->~T();
				},
				[](void* dest, const void* source)
				{
					new(dest) T(*static_cast<const T*>(source)); 
				},
				[](void* dest, void* source) 
				{
					new(dest) T(std::move(*static_cast<T*>(source)));
				},
				[](void* dest, void* source) 
				{
					new(dest) T(std::move(*static_cast<T*>(source)));
					static_cast<T*>(source)->~T(); 
				}
			};
		}

		SC_CONSTEX ComponentMetadata Metadata()
		{
			return Metadata(ComponentTraits<T>::storage);
		}
	};
}
