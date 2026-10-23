#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* CRTP mixin giving T a uniform OnDetach() entry point that forwards
	* to T::Detach(), the counterpart of Attachable: the place where a
	* component undoes, on the moment it is removed from its actor, what
	* its OnAttach prepared there. Lets ComponentBehaviour call OnDetach()
	* through a type-erased function pointer without needing to know
	* whether T actually implements Detach().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* T に統一された OnDetach() エントリポイントを与える CRTP ミックスイン。
	* T::Detach() へ転送する。Attachable と対になるもので、コンポーネントが
	* actor から外される瞬間に、OnAttach がそこに用意したものを片付ける
	* 場所になる。ComponentBehaviour が、T が実際に Detach() を実装して
	* いるかを知らずとも、型消去された関数ポインタ経由で OnDetach() を
	* 呼び出せるようにする。
	*/
	template<typename T>
	class Detachable
	{
	public:
		/**
		* [EN]
		* Forwards to T::Detach().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T::Detach() へ転送する。
		*/
		void OnDetach()
		{
			static_cast<T*>(this)->Detach();
		}
	};

	/// [EN] Satisfied when T::OnDetach() is callable and returns void (i.e. T implements Detach() via the Detachable mixin, or defines OnDetach() itself).
	/// [JP] T::OnDetach() が呼び出し可能で void を返す場合に満たされる（すなわち T が Detachable ミックスイン経由で Detach() を実装しているか、OnDetach() を自分で定義している）。
	template<typename T>
	concept HasDetach = requires(T & a)
	{
		{ a.OnDetach() } -> std::same_as<void>;
	};
}
