#pragma once
#include <FoundationEngine/Utility/ResourcePtr.h>

namespace JPH
{
	class PhysicsSystem;
}

namespace SeedCore
{
	class World;

	/**
	* [EN]
	* Collects Jolt body-contact callbacks and dispatches SeedCore collision
	* and trigger events on the active World.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt のボディ接触コールバックを収集し、アクティブな World へ
	* SeedCore の衝突・トリガーイベントを通知する。
	*/
	class JoltContactListener final
	{
	public:
		/**
		* [EN]
		* Creates a listener with no contacts recorded yet.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 接触をまだ何も記録していないリスナーを作る。
		*/
		JoltContactListener();

		/**
		* [EN]
		* Defined in the .cpp, where Impl is a complete type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Impl が完全型になる .cpp 側で定義する。
		*/
		~JoltContactListener();

		/// [EN] Jolt holds a raw pointer to the Impl once attached, so the listener is neither copied nor reassigned.
		/// [JP] 取り付けた後は Jolt が Impl の生ポインタを持つので、リスナーはコピーも代入もしない。
		JoltContactListener(const JoltContactListener&) = delete;
		JoltContactListener& operator=(const JoltContactListener&) = delete;

	public:
		/**
		* [EN]
		* Registers this listener on the physics system, so body contacts
		* start arriving here.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 物理システムにこのリスナーを登録し、ボディの接触がここへ届く
		* ようにする。
		*/
		void Attach(JPH::PhysicsSystem& physicsSystem);

		/**
		* [EN]
		* Decides which body pairs that lost their last contact really
		* exited, then drains queued contacts and dispatches their
		* collision or trigger events on world.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 最後の接触を失ったボディペアのうち本当に離れたものを判定し、その後
		* キュー内の接触を取り出して衝突またはトリガーイベントを world へ通知する。
		*/
		void DispatchEvent(JPH::PhysicsSystem& physicsSystem, World& world);

	private:
		class Impl;

		/// [EN] The listener Jolt actually calls, owned through a pointer so Jolt's headers stay out of this one.
		/// [JP] Jolt が実際に呼ぶリスナー。Jolt のヘッダをこのヘッダから外すため、ポインタ越しに所有する。
		ResourcePtr<Impl> impl_;
	};
}
