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
		JoltContactListener();
		~JoltContactListener();

		JoltContactListener(const JoltContactListener&) = delete;
		JoltContactListener& operator=(const JoltContactListener&) = delete;

	public:
		void Attach(JPH::PhysicsSystem& physicsSystem);

		void DispatchEvent(JPH::PhysicsSystem& physicsSystem, World& world);

	private:
		class Impl;

		ResourcePtr<Impl> impl_;
	};
}
