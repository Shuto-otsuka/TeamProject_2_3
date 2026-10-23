#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	/// [EN] Forward declaration of the actor passed as the other party; passed by reference, so its definition is not needed here.
	/// [JP] 相手として渡す actor の前方宣言。参照で渡すので、ここでは定義が要らない。
	class Actor;

	/**
	* [EN]
	* CRTP mixin giving T uniform OnTriggerEnter/Stay/Exit() entry
	* points that forward to T::TriggerEnter/Stay/Exit(). Lets
	* ComponentBehaviour call these through type-erased function pointers
	* without needing to know whether T actually implements them.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* T に統一された OnTriggerEnter/Stay/Exit() エントリポイントを
	* 与える CRTP ミックスイン。T::TriggerEnter/Stay/Exit() へ転送する。
	* ComponentBehaviour が、T が実際にこれらを実装しているかを知らずとも、
	* 型消去された関数ポインタ経由で呼び出せるようにする。
	*/
	template<typename T>
	class Triggerable
	{
	public:
		/**
		* [EN]
		* Forwards to T::TriggerEnter(other).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T::TriggerEnter(other) へ転送する。
		*/
		void OnTriggerEnter(const Actor& other)
		{
			static_cast<T*>(this)->TriggerEnter(other);
		}

		/**
		* [EN]
		* Forwards to T::TriggerStay(other).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T::TriggerStay(other) へ転送する。
		*/
		void OnTriggerStay(const Actor& other)
		{
			static_cast<T*>(this)->TriggerStay(other);
		}

		/**
		* [EN]
		* Forwards to T::TriggerExit(other).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T::TriggerExit(other) へ転送する。
		*/
		void OnTriggerExit(const Actor& other)
		{
			static_cast<T*>(this)->TriggerExit(other);
		}
	};

	/// [EN] Satisfied when T::OnTriggerEnter(const Actor&) is callable and returns void (i.e. T implements TriggerEnter() via the Triggerable mixin).
	/// [JP] T::OnTriggerEnter(const Actor&) が呼び出し可能で void を返す場合に満たされる（すなわち T が Triggerable ミックスイン経由で TriggerEnter() を実装している）。
	template<typename T>
	concept HasTriggerEnter = requires(T& a, const Actor& other)
	{
		{ a.OnTriggerEnter(other) } -> std::same_as<void>;
	};

	/// [EN] Satisfied when T::OnTriggerStay(const Actor&) is callable and returns void (i.e. T implements TriggerStay() via the Triggerable mixin).
	/// [JP] T::OnTriggerStay(const Actor&) が呼び出し可能で void を返す場合に満たされる（すなわち T が Triggerable ミックスイン経由で TriggerStay() を実装している）。
	template<typename T>
	concept HasTriggerStay = requires(T& a, const Actor& other)
	{
		{ a.OnTriggerStay(other) } -> std::same_as<void>;
	};

	/// [EN] Satisfied when T::OnTriggerExit(const Actor&) is callable and returns void (i.e. T implements TriggerExit() via the Triggerable mixin).
	/// [JP] T::OnTriggerExit(const Actor&) が呼び出し可能で void を返す場合に満たされる（すなわち T が Triggerable ミックスイン経由で TriggerExit() を実装している）。
	template<typename T>
	concept HasTriggerExit = requires(T& a, const Actor& other)
	{
		{ a.OnTriggerExit(other) } -> std::same_as<void>;
	};
}
