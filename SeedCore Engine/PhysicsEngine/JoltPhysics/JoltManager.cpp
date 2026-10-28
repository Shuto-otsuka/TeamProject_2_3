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

#if SC_DEBUG
#pragma comment(lib, "../External/JoltPhysics/Lib/Debug/Jolt.lib")
#else
#pragma comment(lib, "../External/JoltPhysics/Lib/Release/Jolt.lib")
#endif

namespace SeedCore
{
	class JoltManager::Impl final
	{
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
		JoltShapePool shapePool_;

		JoltConstraintPool constraintPool_;

		ResourcePtr<JoltExecutorBridge> executor_;

		ResourcePtr<JPH::TempAllocatorImpl> tempAllocator_;

		JoltLayerFilters layerFilters_;

		JPH::PhysicsSystem physicsSystem_;

		JoltContactListener contactListener_;
	};

	/**
	* [EN]
	* Initializes Jolt and connects it to the supplied job executor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt を初期化し、指定されたジョブエグゼキューターへ接続する。
	*/
	Bool JoltManager::Impl::Initialize(JobExecutor& executor)
	{
		/// [EN] Register Jolt's allocator, factory and runtime types before creating physics resources.
		/// [JP] 物理リソースの生成前に、Jolt のアロケーター、ファクトリー、実行時型を登録する。
		JPH::RegisterDefaultAllocator();
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		constexpr JPH::uint tempAllocatorSize = 100 * 1024 * 1024;
		tempAllocator_ = MakePtr<JPH::TempAllocatorImpl>(tempAllocatorSize);

		executor_ = MakePtr<JoltExecutorBridge>(executor);

		layerFilters_.Initialize(physicsSystem_);
		contactListener_.Attach(physicsSystem_);

		physicsSystem_.SetSimCollideBodyVsBody([](const JPH::Body& body1, const JPH::Body& body2, JPH::Mat44Arg centerOfMassTransform1, JPH::Mat44Arg centerOfMassTransform2, JPH::CollideShapeSettings& collideShapeSettings, JPH::CollideShapeCollector& collector, const JPH::ShapeFilter& shapeFilter)
		{
			collideShapeSettings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
			JPH::PhysicsSystem::sDefaultSimCollideBodyVsBody(body1, body2, centerOfMassTransform1, centerOfMassTransform2, collideShapeSettings, collector, shapeFilter);
		});

		return true;
	}

	/**
	* [EN]
	* Advances the physics simulation and dispatches queued contact events.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 物理シミュレーションを進め、キュー内の接触イベントを通知する。
	*/
	void JoltManager::Impl::Execute(Float elapsedTime, World& world)
	{
		physicsSystem_.Update(elapsedTime, 1, tempAllocator_.get(), executor_->Native());
		contactListener_.DispatchEvent(physicsSystem_, world);
	}

	/**
	* [EN]
	* Releases physics resources and unregisters Jolt types.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 物理リソースを解放し、Jolt の型登録を解除する。
	*/
	void JoltManager::Impl::Finalize()
	{
		/// [EN] Shapes must be released before UnregisterTypes.
		/// [JP] Shapeの解放はUnregisterTypesより前に行う必要がある。
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

		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	/**
	* [EN]
	* Returns the shape pool owned by this manager.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このマネージャーが所有する形状プールを返す。
	*/
	JoltShapePool& JoltManager::Impl::ShapePool()
	{
		return shapePool_;
	}

	/**
	* [EN]
	* Returns the constraint pool owned by this manager.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このマネージャーが所有する拘束プールを返す。
	*/
	JoltConstraintPool& JoltManager::Impl::ConstraintPool()
	{
		return constraintPool_;
	}

	/**
	* [EN]
	* Returns Jolt's body interface.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt のボディインターフェースを返す。
	*/
	JPH::BodyInterface& JoltManager::Impl::BodyInterface()
	{
		return physicsSystem_.GetBodyInterface();
	}

	/**
	* [EN]
	* Returns the owned Jolt physics system.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 所有する Jolt 物理システムを返す。
	*/
	JPH::PhysicsSystem& JoltManager::Impl::PhysicsSystem()
	{
		return physicsSystem_;
	}

	/**
	* [EN]
	* Returns the temporary allocator used during physics updates.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 物理更新中に使う一時アロケーターを返す。
	*/
	JPH::TempAllocator& JoltManager::Impl::PhysicsAllocator()
	{
		return *tempAllocator_;
	}

	JoltManager::JoltManager() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	JoltManager::~JoltManager() = default;

	Bool JoltManager::Initialize(JobExecutor& executor)
	{
		if (!impl_->Initialize(executor))
		{
			return false;
		}

		Gateway::BindJoltManager(this);
		return true;
	}

	void JoltManager::Execute(Float elapsedTime, World& world)
	{
		impl_->Execute(elapsedTime, world);
	}

	void JoltManager::Finalize()
	{
		impl_->Finalize();
	}

	JoltShapePool& JoltManager::ShapePool()
	{
		return impl_->ShapePool();
	}

	JoltConstraintPool& JoltManager::ConstraintPool()
	{
		return impl_->ConstraintPool();
	}

	JPH::BodyInterface& JoltManager::BodyInterface()
	{
		return impl_->BodyInterface();
	}

	JPH::PhysicsSystem& JoltManager::PhysicsSystem()
	{
		return impl_->PhysicsSystem();
	}

	JPH::TempAllocator& JoltManager::PhysicsAllocator()
	{
		return impl_->PhysicsAllocator();
	}

}
