#pragma once
#include <FoundationEngine/Utility/Keyword.h>
#include <FoundationEngine/Utility/ResourcePtr.h>

namespace JPH
{
	class BodyInterface;
	class PhysicsSystem;
	class TempAllocator;
}

namespace SeedCore
{
	class JobExecutor;
	class World;
	class JoltShapePool;
	class JoltConstraintPool;

	/**
	* [EN]
	* Owns and coordinates the Jolt physics backend.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt 物理バックエンドを所有し、連携させる。
	*/
	class SEEDCORE_API JoltManager final
	{
	public:
		JoltManager();
		~JoltManager();

		JoltManager(const JoltManager&) = delete;
		JoltManager& operator=(const JoltManager&) = delete;

	public:
		Bool Initialize(JobExecutor& executor);

		void Execute(Float elapsedTime, World& world);

		void Finalize();

		JoltShapePool& ShapePool();

		JoltConstraintPool& ConstraintPool();

		JPH::BodyInterface& BodyInterface();

		JPH::PhysicsSystem& PhysicsSystem();

		JPH::TempAllocator& PhysicsAllocator();

	private:
		class Impl;

		ResourcePtr<Impl> impl_;
	};
}
