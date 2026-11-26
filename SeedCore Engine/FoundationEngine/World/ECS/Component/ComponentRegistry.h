#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Component/ComponentBehaviour.h>

namespace SeedCore
{
	template<typename T, typename = void>
	struct HasComponentBehaviourTag : std::false_type {};

	template<typename T>
	struct HasComponentBehaviourTag<T, std::void_t<typename T::is_component_behaviour_tag>> : std::true_type {};

	template<typename T>
	struct ComponentType
	{
		static inline ComponentID id_ = nullptr;

		SC_CONSTEX ComponentMetadata metadata_ = Component<T>::Metadata();

		SC_CONSTEX ComponentHooks hooks_ = []()
			{
				ComponentHooks hooks;

				if constexpr (HasComponentBehaviourTag<T>::value)
				{
					hooks.upcast_ = [](void* component) -> ComponentBehaviour* { return static_cast<T*>(component); };

					if constexpr (HasAttach<T>)
					{
						hooks.attach_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnAttach();
							};
					}
					if constexpr (HasDetach<T>)
					{
						hooks.detach_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnDetach();
							};
					}
					if constexpr (HasAwake<T>)
					{
						hooks.awake_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnAwake();
							};
					}
					if constexpr (HasStart<T>)
					{
						hooks.start_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnStart();
							};
					}
					if constexpr (HasTick<T>)
					{
						hooks.tick_ = [](ComponentBehaviour* component, Float elapsedTime)
							{
								static_cast<T*>(component)->OnTick(elapsedTime);
							};
					}
					if constexpr (HasFixedTick<T>)
					{
						hooks.fixedTick_ = [](ComponentBehaviour* component, Float elapsedTime)
							{
								static_cast<T*>(component)->OnFixedTick(elapsedTime);
							};
					}
					if constexpr (HasLateTick<T>)
					{
						hooks.lateTick_ = [](ComponentBehaviour* component, Float elapsedTime)
							{
								static_cast<T*>(component)->OnLateTick(elapsedTime);
							};
					}
					if constexpr (HasEditorTick<T>)
					{
						hooks.editorTick_ = [](ComponentBehaviour* component, Float elapsedTime)
							{
								static_cast<T*>(component)->OnEditorTick(elapsedTime);
							};
					}
					if constexpr (HasDestroy<T>)
					{
						hooks.destroy_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnDestroy();
							};
					}
					if constexpr (HasInspectorGUI<T>)
					{
						hooks.inspectorGUI_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnInspectorGUI();
							};
					}
					if constexpr (HasCollisionEnter<T>)
					{
						hooks.collisionEnter_ = [](ComponentBehaviour* component, const Actor& other)
							{
								static_cast<T*>(component)->OnCollisionEnter(other);
							};
					}
					if constexpr (HasCollisionStay<T>)
					{
						hooks.collisionStay_ = [](ComponentBehaviour* component, const Actor& other)
							{
								static_cast<T*>(component)->OnCollisionStay(other);
							};
					}
					if constexpr (HasCollisionExit<T>)
					{
						hooks.collisionExit_ = [](ComponentBehaviour* component, const Actor& other)
							{
								static_cast<T*>(component)->OnCollisionExit(other);
							};
					}
					if constexpr (HasTriggerEnter<T>)
					{
						hooks.triggerEnter_ = [](ComponentBehaviour* component, const Actor& other)
							{
								static_cast<T*>(component)->OnTriggerEnter(other);
							};
					}
					if constexpr (HasTriggerStay<T>)
					{
						hooks.triggerStay_ = [](ComponentBehaviour* component, const Actor& other)
							{
								static_cast<T*>(component)->OnTriggerStay(other);
							};
					}
					if constexpr (HasTriggerExit<T>)
					{
						hooks.triggerExit_ = [](ComponentBehaviour* component, const Actor& other)
							{
								static_cast<T*>(component)->OnTriggerExit(other);
							};
					}
					if constexpr (HasRevision<T>)
					{
						hooks.revision_ = [](ComponentBehaviour* component)
							{
								static_cast<T*>(component)->OnRevision();
							};
					}
				}
				return hooks;
			}();
	};

	class SEEDCORE_API ComponentRegistry
	{
	public:
		template<typename T>
		static void Register(String name, String category = String::intern("Custom"))
		{
			static_assert(!(HasComponentBehaviourTag<T>::value && ComponentTraits<T>::storage == ComponentStorage::Archetype), "ComponentBehaviour は Archetype に格納できません。SparseSet にしてください。");

			const ComponentHooks* hooks = nullptr;
			if constexpr (HasComponentBehaviourTag<T>::value)
			{
				hooks = &ComponentType<T>::hooks_;
			}

			ComponentID id = Register(name, category, &ComponentType<T>::metadata_, hooks);
			if (!id)
			{
				return;
			}

			ComponentType<T>::id_ = id;
		}

		static ComponentID Register(String name, String category, const ComponentMetadata* metadata, const ComponentHooks* hooks);

		static void Unregister(ComponentID id);

		template<typename T>
		static ComponentID GetComponentID()
		{
			if (!ComponentType<T>::id_ || !ComponentType<T>::id_->metadata_)
			{
				return nullptr;
			}
			return ComponentType<T>::id_;
		}

		static ComponentID GetComponentID(String name);

		static String Name(ComponentID id);

		static std::span<const ComponentID> Registry();

		static const ComponentMetadata& Get(ComponentID id);

		static Size GetID(ComponentID id);

		static Size ComponentSize(ComponentID id);

		static Size ComponentAlignment(ComponentID id);

	private:
		static ComponentRecord* ResolveRecord(String name);

		static void* ResolveModule(const void* address);

	private:
		static inline DynamicArray<ResourcePtr<StaticArray<ComponentRecord, 64>>> pages_;

		static inline DynamicArray<ComponentID> records_;
	};
}

#define SEED_STORAGE_SPEC(Type, ...) \
	__VA_OPT__(constexpr SeedCore::ComponentStorage DeclaredComponentStorage(Type*) { return __VA_ARGS__; })

#define REGISTER_COMPONENT_1(Type) \
	static const SeedCore::Bool Type##_registered = []() { \
		SeedCore::ComponentRegistry::Register<Type>(SeedCore::String::intern(#Type)); \
		return true; \
	}()

#define REGISTER_COMPONENT_2(Type, Category) \
	static const SeedCore::Bool Type##_registered = []() { \
		SeedCore::ComponentRegistry::Register<Type>(SeedCore::String::intern(#Type), SeedCore::String(Category)); \
		return true; \
	}()

#define REGISTER_COMPONENT_3(Type, Category, Storage) \
	SEED_STORAGE_SPEC(Type, Storage) \
	static const SeedCore::Bool Type##_registered = []() { \
		SeedCore::ComponentRegistry::Register<Type>(SeedCore::String::intern(#Type), SeedCore::String(Category)); \
		return true; \
	}()

#define REGISTER_COMPONENT_GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define REGISTER_COMPONENT(...) REGISTER_COMPONENT_GET_MACRO(__VA_ARGS__, REGISTER_COMPONENT_3, REGISTER_COMPONENT_2, REGISTER_COMPONENT_1)(__VA_ARGS__)