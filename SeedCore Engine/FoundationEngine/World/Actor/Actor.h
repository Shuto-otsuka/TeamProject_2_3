#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Component/ComponentBehaviour.h>
#include <FoundationEngine/World/ECS/Component/ComponentRegistry.h>
#include <FoundationEngine/World/Service.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <FoundationEngine/Log/Warning.h>

namespace SeedCore
{
	class World;

	class Actor
	{
	private:
		friend class ActorAccessor;

		SEEDCORE_API Actor(World& world, Entity entity);

	public:
		Actor() = default;

		SEEDCORE_API explicit operator Bool()const;

		SEEDCORE_API Bool operator==(const Actor& other)const;

	public:
		template<typename T>
		T* AddComponent()
		{
			return static_cast<T*>(AddComponent(ComponentRegistry::GetComponentID<T>()));
		}

		template<typename T>
			requires (!std::derived_from<T, ComponentBehaviour>)
		T* AddComponent(const T& value)
		{
			T* component = AddComponent<T>();
			if (component)
			{
				*component = value;
			}
			return component;
		}

		SEEDCORE_API void* AddComponent(ComponentID id);

		template<typename T>
		T* GetComponent()const
		{
			return static_cast<T*>(GetComponent(ComponentRegistry::GetComponentID<T>()));
		}

		SEEDCORE_API void* GetComponent(ComponentID id)const;

		template<typename T>
		Bool HasComponent()const
		{
			return HasComponent(ComponentRegistry::GetComponentID<T>());
		}

		SEEDCORE_API Bool HasComponent(ComponentID id)const;

		template<typename T>
		void RemoveComponent()
		{
			RemoveComponent(ComponentRegistry::GetComponentID<T>());
		}

		SEEDCORE_API void RemoveComponent(ComponentID id);

		template<typename T>
		T& Field(const String& componentName, const String& fieldName)const
		{
			static T fallback{};
			fallback = T{};

			ComponentID id = ComponentRegistry::GetComponentID(componentName);
			void* data = id ? GetComponent(id) : nullptr;
			auto reflection = ReflectionRegistry::GetRegistry().find(componentName);
			if (data == nullptr || reflection == ReflectionRegistry::GetRegistry().end())
			{
				SC_LOG_WARNING("Field: この actor は {} を持っていません", componentName.str());
				return fallback;
			}

			DynamicArray<FieldInfo> fields;
			reflection->second(data, fields);
			for (const FieldInfo& field : fields)
			{
				if (field.name_ == fieldName && field.type_ == AttributeTraits<T>::type)
				{
					return *static_cast<T*>(field.directPtr_ ? field.directPtr_ : static_cast<Uint8*>(data) + field.offset_);
				}
			}

			SC_LOG_WARNING("Field: {} に {} という名前でこの型のフィールドはありません", componentName.str(), fieldName.str());
			return fallback;
		}

		template<typename Result = void, typename... Arguments>
		Result Function(const String& componentName, const String& functionName, const Arguments&... arguments)const
		{
			std::conditional_t<std::is_void_v<Result>, Int, Result> result{};

			ComponentID id = ComponentRegistry::GetComponentID(componentName);
			void* data = id ? GetComponent(id) : nullptr;
			auto functions = FunctionRegistry::GetRegistry().find(componentName);
			if (data == nullptr || functions == FunctionRegistry::GetRegistry().end())
			{
				SC_LOG_WARNING("Function: この actor は {} を持っていないか、{} に呼べる関数がありません", componentName.str(), componentName.str());
			}
			else
			{
				StaticArray<AttributeType, sizeof...(Arguments)> parameters{ AttributeTraits<Arguments>::type... };
				auto function = std::ranges::find_if(functions->second, [&functionName](const FunctionInfo& info) { return info.name_ == functionName; });
				if (function == functions->second.end())
				{
					SC_LOG_WARNING("Function: {} に {} という関数はありません", componentName.str(), functionName.str());
				}
				else if (function->result_ != AttributeTraits<Result>::type || !std::ranges::equal(function->parameters_, parameters))
				{
					SC_LOG_WARNING("Function: {}.{} の引数か戻り値の型が合いません", componentName.str(), functionName.str());
				}
				else
				{
					void* pointers[sizeof...(Arguments) + 1]{ const_cast<void*>(static_cast<const void*>(&arguments))... };
					function->invoke_(data, pointers, std::is_void_v<Result> ? nullptr : &result);
				}
			}

			if constexpr (!std::is_void_v<Result>)
			{
				return result;
			}
		}

	public:
		SEEDCORE_API World& GetWorld()const;

		template<typename T>
		T* GetService()const
		{
			return static_cast<T*>(GetService(ServiceType<T>::name_));
		}

	public:
		SEEDCORE_API void Parent(Actor parent);

		SEEDCORE_API Actor Parent()const;

		SEEDCORE_API DynamicArray<Actor> Children()const;

		SEEDCORE_API void Move(Actor after);

	public:
		SEEDCORE_API const Matrix& WorldMatrix()const;

		SEEDCORE_API void WorldMatrix(const Matrix& matrix);

	public:
		SEEDCORE_API Bool Active()const;

		SEEDCORE_API void Active(Bool active);

	public:
		SEEDCORE_API void AddTag(String tag);

		SEEDCORE_API Bool HasTag(String tag)const;

		SEEDCORE_API void RemoveTag(String tag);

	public:
		SEEDCORE_API void Layer(Size index);

		SEEDCORE_API void Layer(const String& name);

		SEEDCORE_API Size Layer()const;

		SEEDCORE_API const String& LayerName()const;

	private:
		SEEDCORE_API void* GetService(const String& type)const;

	private:
		World* world_ = nullptr;

		Entity entity_;
	};
}
