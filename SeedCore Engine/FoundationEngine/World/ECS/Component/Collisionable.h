#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	/// [EN] Forward declaration of the actor passed as the other party; passed by reference, so its definition is not needed here.
	/// [JP] 相手として渡す actor の前方宣言。参照で渡すので、ここでは定義が要らない。
	class Actor;

	/**
	* [EN]
	* CRTP mixin giving T uniform OnCollisionEnter/Stay/Exit() entry
	* points that forward to T::CollisionEnter/Stay/Exit(). Lets
	* ComponentBehaviour call these through type-erased function pointers
	* without needing to know whether T actually implements them.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* T に統一された OnCollisionEnter/Stay/Exit() エントリポイントを
	* 与える CRTP ミックスイン。T::CollisionEnter/Stay/Exit() へ転送する。
	* ComponentBehaviour が、T が実際にこれらを実装しているかを知らずとも、
	* 型消去された関数ポインタ経由で呼び出せるようにする。
	*/
	template<typename T>
	class Collisionable
	{
	public:
		/**
		* [EN]
		* Forwards to T::CollisionEnter(other).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T::CollisionEnter(other) へ転送する。
		*/
		void OnCollisionEnter(const Actor& other)
		{
			static_cast<T*>(this)->CollisionEnter(other);
		}

		/**
		* [EN]
		* Forwards to T::CollisionStay(other).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T::CollisionStay(other) へ転送する。
		*/
		void OnCollisionStay(const Actor& other)
		{
			static_cast<T*>(this)->CollisionStay(other);
		}

		/**
		* [EN]
		* Forwards to T::CollisionExit(other).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T::CollisionExit(other) へ転送する。
		*/
		void OnCollisionExit(const Actor& other)
		{
			static_cast<T*>(this)->CollisionExit(other);
		}
	};

	/// [EN] Satisfied when T::OnCollisionEnter(const Actor&) is callable and returns void (i.e. T implements CollisionEnter() via the Collisionable mixin).
	/// [JP] T::OnCollisionEnter(const Actor&) が呼び出し可能で void を返す場合に満たされる（すなわち T が Collisionable ミックスイン経由で CollisionEnter() を実装している）。
	template<typename T>
	concept HasCollisionEnter = requires(T& a, const Actor& other)
	{
		{ a.OnCollisionEnter(other) } -> std::same_as<void>;
	};

	/// [EN] Satisfied when T::OnCollisionStay(const Actor&) is callable and returns void (i.e. T implements CollisionStay() via the Collisionable mixin).
	/// [JP] T::OnCollisionStay(const Actor&) が呼び出し可能で void を返す場合に満たされる（すなわち T が Collisionable ミックスイン経由で CollisionStay() を実装している）。
	template<typename T>
	concept HasCollisionStay = requires(T& a, const Actor& other)
	{
		{ a.OnCollisionStay(other) } -> std::same_as<void>;
	};

	/// [EN] Satisfied when T::OnCollisionExit(const Actor&) is callable and returns void (i.e. T implements CollisionExit() via the Collisionable mixin).
	/// [JP] T::OnCollisionExit(const Actor&) が呼び出し可能で void を返す場合に満たされる（すなわち T が Collisionable ミックスイン経由で CollisionExit() を実装している）。
	template<typename T>
	concept HasCollisionExit = requires(T& a, const Actor& other)
	{
		{ a.OnCollisionExit(other) } -> std::same_as<void>;
	};
}
