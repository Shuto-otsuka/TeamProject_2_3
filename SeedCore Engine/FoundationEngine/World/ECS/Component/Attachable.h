#pragma once

namespace SeedCore
{
	/**
	* [EN]
	* CRTP mixin giving T a uniform OnAttach() entry point that forwards
	* to T::Attach(). OnAttach runs once, the moment the component is added
	* to an actor (after it has been wired to its world and entity, and
	* whether or not the world is playing), so it is where a component
	* prepares what it needs on the same actor, such as Rigidbody adding
	* the velocities it keeps in step with its body. Lets
	* ComponentBehaviour call OnAttach() through a type-erased function
	* pointer without needing to know whether T actually implements
	* Attach().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* T に統一された OnAttach() エントリポイントを与える CRTP ミックスイン。
	* T::Attach() へ転送する。OnAttach は、コンポーネントが actor に追加
	* された瞬間に1回だけ走る（World とエンティティへ結び付けられた後で、
	* World がプレイ中かどうかは問わない）。そのため、同じ actor の上に
	* 自分が必要とするものを用意する場所になる。例えば Rigidbody が、
	* ボディと同期させる速度を追加する。ComponentBehaviour が、T が実際に
	* Attach() を実装しているかを知らずとも、型消去された関数ポインタ経由で
	* OnAttach() を呼び出せるようにする。
	*/
	template<typename T>
	class Attachable
	{
	public:
		/**
		* [EN]
		* Forwards to T::Attach().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* T::Attach() へ転送する。
		*/
		void OnAttach()
		{
			static_cast<T*>(this)->Attach();
		}
	};

	/// [EN] Satisfied when T::OnAttach() is callable and returns void (i.e. T implements Attach() via the Attachable mixin, or defines OnAttach() itself).
	/// [JP] T::OnAttach() が呼び出し可能で void を返す場合に満たされる（すなわち T が Attachable ミックスイン経由で Attach() を実装しているか、OnAttach() を自分で定義している）。
	template<typename T>
	concept HasAttach = requires(T & a)
	{
		{ a.OnAttach() } -> std::same_as<void>;
	};
}
