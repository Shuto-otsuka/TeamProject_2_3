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
	* [JP]
	* 物理エンジン（Jolt）を始めたり、毎フレーム動かしたり、終わらせたりする係。
	*
	* @brief 物理エンジンを動かす係
	*
	* [EN]
	* Starts the physics engine (Jolt), runs it every frame and shuts it down.
	*
	* @brief Runs the physics engine
	*/
	class SEEDCORE_API JoltManager final
	{
	public:
		/**
		* [JP]
		* 中身を作る。
		*
		* @brief 中身を作る
		*
		* [EN]
		* Makes the inner parts.
		*
		* @brief Makes the inner parts
		*/
		JoltManager();

		/**
		* [JP]
		* 中身を片付ける。
		*
		* @brief 中身を片付ける
		*
		* [EN]
		* Cleans up the inner parts.
		*
		* @brief Cleans up the inner parts
		*/
		~JoltManager();

		JoltManager(const JoltManager&) = delete;
		JoltManager& operator=(const JoltManager&) = delete;

	public:
		/**
		* [JP]
		* 物理エンジンを始める。
		*
		* @brief  始める
		* @param  executor 物理の仕事を流す先
		* @return うまくいったら true
		*
		* [EN]
		* Starts the physics engine.
		*
		* @brief  Starts
		* @param  executor Where physics jobs are sent
		* @return True if it worked
		*/
		Bool Initialize(JobExecutor& executor);

		/**
		* [JP]
		* 物理を1回分進めて、ぶつかったことを知らせる。
		*
		* @brief 物理を1回分進める
		* @param elapsedTime 進める時間
		* @param world       知らせる世界
		*
		* [EN]
		* Steps the physics once and reports what hit.
		*
		* @brief Steps the physics
		* @param elapsedTime How much time to step
		* @param world       The world to report to
		*/
		void Execute(Float elapsedTime, World& world);

		/**
		* [JP]
		* 物理エンジンを終わらせる。
		*
		* @brief 終わらせる
		*
		* [EN]
		* Shuts the physics engine down.
		*
		* @brief Shuts down
		*/
		void Finalize();

		/**
		* [JP]
		* 形をまとめて持つ係を返す。
		*
		* @brief  形の係を返す
		* @return 形の係
		*
		* [EN]
		* Returns the part that holds the shapes.
		*
		* @brief  Returns the shape pool
		* @return The shape pool
		*/
		JoltShapePool& ShapePool();

		/**
		* [JP]
		* つなぎをまとめて持つ係を返す。
		*
		* @brief  つなぎの係を返す
		* @return つなぎの係
		*
		* [EN]
		* Returns the part that holds the joints.
		*
		* @brief  Returns the joint pool
		* @return The joint pool
		*/
		JoltConstraintPool& ConstraintPool();

		/**
		* [JP]
		* 体を作ったり動かしたりする窓口を返す。
		*
		* @brief  体の窓口を返す
		* @return 体の窓口
		*
		* [EN]
		* Returns the door for making and moving bodies.
		*
		* @brief  Returns the body interface
		* @return The body interface
		*/
		JPH::BodyInterface& BodyInterface();

		/**
		* [JP]
		* 物理エンジンそのものを返す。
		*
		* @brief  物理エンジンを返す
		* @return 物理エンジン
		*
		* [EN]
		* Returns the physics engine itself.
		*
		* @brief  Returns the physics engine
		* @return The physics engine
		*/
		JPH::PhysicsSystem& PhysicsSystem();

		/**
		* [JP]
		* 物理の計算で一時的に使うメモリを返す。
		*
		* @brief  一時メモリを返す
		* @return 一時メモリ
		*
		* [EN]
		* Returns the scratch memory used by the physics math.
		*
		* @brief  Returns the scratch memory
		* @return The scratch memory
		*/
		JPH::TempAllocator& PhysicsAllocator();

	private:
		class Impl;

		/// [JP] 中身
		/// [EN] The inner parts
		ResourcePtr<Impl> impl_;
	};
}
