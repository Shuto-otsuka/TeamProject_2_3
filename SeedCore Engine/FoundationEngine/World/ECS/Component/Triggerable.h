#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	class Actor;

	template<typename T>
	class Triggerable
	{
	public:
		void OnTriggerEnter(const Actor& other)
		{
			static_cast<T*>(this)->TriggerEnter(other);
		}

		void OnTriggerStay(const Actor& other)
		{
			static_cast<T*>(this)->TriggerStay(other);
		}

		void OnTriggerExit(const Actor& other)
		{
			static_cast<T*>(this)->TriggerExit(other);
		}
	};

	template<typename T>
	concept HasTriggerEnter = requires(T& a, const Actor& other)
	{
		{ a.OnTriggerEnter(other) } -> std::same_as<void>;
	};

	template<typename T>
	concept HasTriggerStay = requires(T& a, const Actor& other)
	{
		{ a.OnTriggerStay(other) } -> std::same_as<void>;
	};

	template<typename T>
	concept HasTriggerExit = requires(T& a, const Actor& other)
	{
		{ a.OnTriggerExit(other) } -> std::same_as<void>;
	};
}
