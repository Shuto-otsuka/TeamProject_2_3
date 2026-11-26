#include <PhysicsEngine/JoltPhysics/JoltConfig.h>
#include <External/JoltPhysics/Jolt/Jolt.h>
#include <External/JoltPhysics/Jolt/RegisterTypes.h>
#include <External/JoltPhysics/Jolt/Core/Factory.h>
#include <External/JoltPhysics/Jolt/Core/TempAllocator.h>
#include <External/JoltPhysics/Jolt/Physics/PhysicsSystem.h>
#include <External/JoltPhysics/Jolt/Physics/Body/Body.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/CollideShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/ShapeFilter.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/Shape.h>
#include <External/JoltPhysics/Jolt/Physics/Constraints/Constraint.h>
#include <PhysicsEngine/JoltPhysics/JoltManager.h>
#include <PhysicsEngine/JoltPhysics/JoltExecutorBridge.h>
#include <PhysicsEngine/JoltPhysics/JoltLayerdef.h>
#include <PhysicsEngine/JoltPhysics/JoltShapePool.h>
#include <PhysicsEngine/JoltPhysics/JoltConstraintPool.h>
#include <PhysicsEngine/JoltPhysics/JoltContactListener.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/Resource/Gateway.h>

/// [JP] Debug と Release で使うライブラリを変える
/// [EN] Link a different library for Debug and Release
#if SC_DEBUG
#pragma comment(lib, "../External/JoltPhysics/Lib/Debug/Jolt.lib")
#else
#pragma comment(lib, "../External/JoltPhysics/Lib/Release/Jolt.lib")
#endif

namespace SeedCore
{
	/**
	* [JP]
	* JoltManager の中身。物理エンジンと、それに使う係をまとめて持つ。
	*
	* @brief JoltManager の中身
	*
	* [EN]
	* The inside of JoltManager; it holds the physics engine and its helpers.
	*
	* @brief The inside of JoltManager
	*/
	class JoltManager::Impl final
	{
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
		/// [JP] 形をまとめて持つ係
		/// [EN] Holds the shapes
		JoltShapePool shapePool_;

		/// [JP] つなぎをまとめて持つ係
		/// [EN] Holds the joints
		JoltConstraintPool constraintPool_;

		/// [JP] 物理の仕事を流すつなぎ役
		/// [EN] Sends physics jobs to the job system
		ResourcePtr<JoltExecutorBridge> executor_;

		/// [JP] 物理の計算で一時的に使うメモリ
		/// [EN] Scratch memory for the physics math
		ResourcePtr<JPH::TempAllocatorImpl> tempAllocator_;

		/// [JP] ぶつかる組み合わせを決める係
		/// [EN] Decides what hits what
		JoltLayerFilters layerFilters_;

		/// [JP] 物理エンジンそのもの
		/// [EN] The physics engine itself
		JPH::PhysicsSystem physicsSystem_;

		/// [JP] ぶつかったことを集める係
		/// [EN] Collects what hit
		JoltContactListener contactListener_;
	};

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
	Bool JoltManager::Impl::Initialize(JobExecutor& executor)
	{
		/// [JP] 物理エンジンが使う型を登録する
		/// [EN] Register the types the physics engine uses
		JPH::RegisterDefaultAllocator();
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		/// [JP] 一時メモリ（100MB）と、仕事を流すつなぎ役を用意する
		/// [EN] Prepare 100 MB of scratch memory and the job go-between
		constexpr JPH::uint tempAllocatorSize = 100 * 1024 * 1024;
		tempAllocator_ = MakePtr<JPH::TempAllocatorImpl>(tempAllocatorSize);

		executor_ = MakePtr<JoltExecutorBridge>(executor);

		/// [JP] ぶつかる組み合わせと、ぶつかりの知らせ先を登録する
		/// [EN] Register the hit rules and where to report hits
		layerFilters_.Initialize(physicsSystem_);
		contactListener_.Attach(physicsSystem_);

		/// [JP] 面の裏側ともぶつかるようにする
		/// [EN] Let things hit the back side of faces too
		physicsSystem_.SetSimCollideBodyVsBody([](const JPH::Body& body1, const JPH::Body& body2, JPH::Mat44Arg centerOfMassTransform1, JPH::Mat44Arg centerOfMassTransform2, JPH::CollideShapeSettings& collideShapeSettings, JPH::CollideShapeCollector& collector, const JPH::ShapeFilter& shapeFilter)
		{
			collideShapeSettings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
			JPH::PhysicsSystem::sDefaultSimCollideBodyVsBody(body1, body2, centerOfMassTransform1, centerOfMassTransform2, collideShapeSettings, collector, shapeFilter);
		});

		return true;
	}

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
	void JoltManager::Impl::Execute(Float elapsedTime, World& world)
	{
		physicsSystem_.Update(elapsedTime, 1, tempAllocator_.get(), executor_->Native());
		contactListener_.DispatchEvent(physicsSystem_, world);
	}

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
	void JoltManager::Impl::Finalize()
	{
		/// [JP] つなぎと形を片付けてから、使った係を消す
		/// [EN] Clear the joints and shapes, then remove the helpers
		constraintPool_.Clear(physicsSystem_);
		shapePool_.Clear();

		if (executor_)
		{
			executor_.reset();
			executor_ = nullptr;
		}

		if (tempAllocator_)
		{
			tempAllocator_.reset();
			tempAllocator_ = nullptr;
		}

		/// [JP] 登録した型を外して、工場を消す
		/// [EN] Unregister the types and remove the factory
		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

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
	JoltShapePool& JoltManager::Impl::ShapePool()
	{
		return shapePool_;
	}

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
	JoltConstraintPool& JoltManager::Impl::ConstraintPool()
	{
		return constraintPool_;
	}

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
	JPH::BodyInterface& JoltManager::Impl::BodyInterface()
	{
		return physicsSystem_.GetBodyInterface();
	}

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
	JPH::PhysicsSystem& JoltManager::Impl::PhysicsSystem()
	{
		return physicsSystem_;
	}

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
	JPH::TempAllocator& JoltManager::Impl::PhysicsAllocator()
	{
		return *tempAllocator_;
	}

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
	JoltManager::JoltManager() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	JoltManager::~JoltManager() = default;

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
	Bool JoltManager::Initialize(JobExecutor& executor)
	{
		/// [JP] 始められたら、どこからでも使えるように登録する
		/// [EN] Once started, register it so anything can use it
		if (!impl_->Initialize(executor))
		{
			return false;
		}

		Gateway::BindJoltManager(this);
		return true;
	}

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
	void JoltManager::Execute(Float elapsedTime, World& world)
	{
		impl_->Execute(elapsedTime, world);
	}

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
	void JoltManager::Finalize()
	{
		impl_->Finalize();
	}

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
	JoltShapePool& JoltManager::ShapePool()
	{
		return impl_->ShapePool();
	}

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
	JoltConstraintPool& JoltManager::ConstraintPool()
	{
		return impl_->ConstraintPool();
	}

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
	JPH::BodyInterface& JoltManager::BodyInterface()
	{
		return impl_->BodyInterface();
	}

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
	JPH::PhysicsSystem& JoltManager::PhysicsSystem()
	{
		return impl_->PhysicsSystem();
	}

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
	JPH::TempAllocator& JoltManager::PhysicsAllocator()
	{
		return impl_->PhysicsAllocator();
	}

}
