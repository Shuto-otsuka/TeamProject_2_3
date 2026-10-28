#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* CRTP mixin giving T a uniform OnRevision() entry point that forwards
	* to T::Revision(). Lets ComponentBehaviour call it through a
	* type-erased function pointer without needing to know whether T
	* actually implements it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* T に統一された OnRevision() エントリポイントを与える CRTP ミックスイン。
	* T::Revision() へ転送する。ComponentBehaviour が、T が実際にこれを
	* 実装しているかを知らずとも、型消去された関数ポインタ経由で
	* 呼び出せるようにする。
	*/
	template<typename T>
	class Revisionable
	{
	public:
		/**
		* [EN]
		* Forwards to T::Revision().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T::Revision() へ転送する。
		*/
		void OnRevision()
		{
			static_cast<T*>(this)->Revision();
		}
	};

	/// [EN] Satisfied when T::OnRevision() is callable and returns void (i.e. T implements Revision() via the Revisionable mixin, or defines OnRevision() itself).
	/// [JP] T::OnRevision() が呼び出し可能で void を返す場合に満たされる（すなわち T が Revisionable ミックスイン経由で Revision() を実装しているか、OnRevision() を自分で定義している）。
	template<typename T>
	concept HasRevision = requires(T & a)
	{
		{ a.OnRevision() } -> std::same_as<void>;
	};
}
