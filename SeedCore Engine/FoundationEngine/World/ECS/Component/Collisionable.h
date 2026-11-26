#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	class Actor;

	template<typename T>
	class Collisionable
	{
	public:
		void OnCollisionEnter(const Actor& other)
		{
			static_cast<T*>(this)->CollisionEnter(other);
		}

		void OnCollisionStay(const Actor& other)
		{
			static_cast<T*>(this)->CollisionStay(other);
		}

		void OnCollisionExit(const Actor& other)
		{
			static_cast<T*>(this)->CollisionExit(other);
		}
	};

	template<typename T>
	concept HasCollisionEnter = requires(T& a, const Actor& other)
	{
		{ a.OnCollisionEnter(other) } -> std::same_as<void>;
	};

	template<typename T>
	concept HasCollisionStay = requires(T& a, const Actor& other)
	{
		{ a.OnCollisionStay(other) } -> std::same_as<void>;
	};

	template<typename T>
	concept HasCollisionExit = requires(T& a, const Actor& other)
	{
		{ a.OnCollisionExit(other) } -> std::same_as<void>;
	};
}
