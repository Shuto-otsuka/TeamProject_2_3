#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* CRTP mixin giving T a uniform OnEditorTick() entry point that
	* forwards to T::EditorTick(). Lets ComponentBehaviour call
	* OnEditorTick() through a type-erased function pointer without needing
	* to know whether T actually implements EditorTick(). EditorTick runs
	* every frame in the editor while not playing, in place of Tick.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* T に統一された OnEditorTick() エントリポイントを与える CRTP
	* ミックスイン。T::EditorTick() へ転送する。ComponentBehaviour が、T が
	* 実際に EditorTick() を実装しているかを知らずとも、型消去された
	* 関数ポインタ経由で OnEditorTick() を呼び出せるようにする。EditorTick は
	* エディターでプレイしていない間、Tick の代わりに毎フレーム呼ばれる。
	*/
	template<typename T>
	class Editable
	{
	public:
		/**
		* [EN]
		* Forwards to T::EditorTick(elapsedTime).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T::EditorTick(elapsedTime) へ転送する。
		*/
		void OnEditorTick(Float elapsedTime)
		{
			static_cast<T*>(this)->EditorTick(elapsedTime);
		}
	};

	/// [EN] Satisfied when T::OnEditorTick(Float) is callable and returns void (i.e. T implements EditorTick() via the Editable mixin).
	/// [JP] T::OnEditorTick(Float) が呼び出し可能で void を返す場合に満たされる（すなわち T が Editable ミックスイン経由で EditorTick() を実装している）。
	template<typename T>
	concept HasEditorTick = requires(T & a, Float elapsedTime)
	{
		{ a.OnEditorTick(elapsedTime) } -> std::same_as<void>;
	};
}