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

#if SC_DEBUG
#pragma comment(lib, "../External/JoltPhysics/Lib/Debug/Jolt.lib")
#else
#pragma comment(lib, "../External/JoltPhysics/Lib/Release/Jolt.lib")
#endif

namespace SeedCore
{
	/**
	* [EN]
	* The Jolt backend behind JoltManager: the physics system, its
	* allocators, job bridge, layer filters, contact listener and pooled
	* shapes and constraints.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* JoltManager の奥にある Jolt のバックエンド。物理システム、アロケーター、
	* ジョブブリッジ、レイヤーフィルター、接触リスナー、形状・拘束プールを持つ。
	*/
	class JoltManager::Impl final
	{
	public:
		/**
		* [EN]
		* Initializes Jolt and connects it to the supplied job executor.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Jolt を初期化し、指定されたジョブエグゼキューターへ接続する。
		*/
		Bool Initialize(JobExecutor& executor);

		/**
		* [EN]
		* Advances the physics simulation and dispatches queued contact events.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 物理シミュレーションを進め、キュー内の接触イベントを通知する。
		*/
		void Execute(Float elapsedTime, World& world);

		/**
		* [EN]
		* Releases physics resources and unregisters Jolt types.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 物理リソースを解放し、Jolt の型登録を解除する。
		*/
		void Finalize();

		/**
		* [EN]
		* Returns the shape pool owned by this manager.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このマネージャーが所有する形状プールを返す。
		*/
		JoltShapePool& ShapePool();

		/**
		* [EN]
		* Returns the constraint pool owned by this manager.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このマネージャーが所有する拘束プールを返す。
		*/
		JoltConstraintPool& ConstraintPool();

		/**
		* [EN]
		* Returns Jolt's body interface.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Jolt のボディインターフェースを返す。
		*/
		JPH::BodyInterface& BodyInterface();

		/**
		* [EN]
		* Returns the owned Jolt physics system.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 所有する Jolt 物理システムを返す。
		*/
		JPH::PhysicsSystem& PhysicsSystem();

		/**
		* [EN]
		* Returns the temporary allocator used during physics updates.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 物理更新中に使う一時アロケーターを返す。
		*/
		JPH::TempAllocator& PhysicsAllocator();

	private:
		/// [EN] Pool that owns reusable Jolt collision shapes.
		/// [JP] 再利用可能な Jolt 衝突形状を所有するプール。
		JoltShapePool shapePool_;

		/// [EN] Pool that owns registered Jolt constraints.
		/// [JP] 登録済み Jolt 拘束を所有するプール。
		JoltConstraintPool constraintPool_;

		/// [EN] Bridge that submits Jolt jobs to SeedCore's executor.
		/// [JP] Jolt ジョブを SeedCore のエグゼキューターへ投入するブリッジ。
		ResourcePtr<JoltExecutorBridge> executor_;

		/// [EN] Temporary memory allocator used by Jolt updates.
		/// [JP] Jolt の更新で使う一時メモリアロケーター。
		ResourcePtr<JPH::TempAllocatorImpl> tempAllocator_;

		/// [EN] Layer filters the physics system refers to; declared before it so they are destroyed after it.
		/// [JP] 物理システムが参照するレイヤーフィルター。物理システムより後に破棄されるよう、先に宣言する。
		JoltLayerFilters layerFilters_;

		/// [EN] Jolt simulation world owned by this manager.
		/// [JP] このマネージャーが所有する Jolt シミュレーションワールド。
		JPH::PhysicsSystem physicsSystem_;

		/// [EN] Listener that queues and dispatches body-contact events.
		/// [JP] ボディ接触イベントをキューへ積み通知するリスナー。
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

		/// [EN] 100 MB of scratch memory for each physics update.
		/// [JP] 物理更新ごとに使う 100 MB の作業メモリ。
		constexpr JPH::uint tempAllocatorSize = 100 * 1024 * 1024;
		tempAllocator_ = MakePtr<JPH::TempAllocatorImpl>(tempAllocatorSize);

		executor_ = MakePtr<JoltExecutorBridge>(executor);

		layerFilters_.Initialize(physicsSystem_);
		contactListener_.Attach(physicsSystem_);

		/// [EN] Bodies also collide with the back faces of mesh triangles, so a body cannot slip through a mesh from behind.
		/// [JP] ボディはメッシュの三角形の裏面とも衝突させ、裏側からメッシュをすり抜けないようにする。
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
		/// [EN] One collision step per update; contact events are dispatched only after the update, on this thread.
		/// [JP] 更新1回につき衝突ステップは1回。接触イベントは更新が終わってから、このスレッドで通知する。
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

	/**
	* [EN]
	* Creates an uninitialized Jolt manager.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 未初期化の Jolt マネージャーを生成する。
	*/
	JoltManager::JoltManager() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	/**
	* [EN]
	* Destroys the manager after explicit finalization. Defined here,
	* where Impl is a complete type.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 明示的な終了処理後にマネージャーを破棄する。Impl が完全型になる
	* ここで定義する。
	*/
	JoltManager::~JoltManager() = default;

	/**
	* [EN]
	* Initializes Jolt, connects it to the supplied job executor, and
	* makes the manager reachable through the Gateway.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt を初期化して指定されたジョブエグゼキューターへ接続し、
	* Gateway からマネージャーへ届くようにする。
	*/
	Bool JoltManager::Initialize(JobExecutor& executor)
	{
		if (!impl_->Initialize(executor))
		{
			return false;
		}

		/// [EN] Published only once Jolt is ready, so nothing reaches a half-initialized manager through the Gateway.
		/// [JP] Jolt の準備が整ってから公開し、Gateway 越しに初期化途中のマネージャーへ届かないようにする。
		Gateway::BindJoltManager(this);
		return true;
	}

	/**
	* [EN]
	* Advances the physics simulation and dispatches queued contact
	* events on world.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 物理シミュレーションを進め、キュー内の接触イベントを world へ通知する。
	*/
	void JoltManager::Execute(Float elapsedTime, World& world)
	{
		impl_->Execute(elapsedTime, world);
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
	void JoltManager::Finalize()
	{
		impl_->Finalize();
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
	JoltShapePool& JoltManager::ShapePool()
	{
		return impl_->ShapePool();
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
	JoltConstraintPool& JoltManager::ConstraintPool()
	{
		return impl_->ConstraintPool();
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
	JPH::BodyInterface& JoltManager::BodyInterface()
	{
		return impl_->BodyInterface();
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
	JPH::PhysicsSystem& JoltManager::PhysicsSystem()
	{
		return impl_->PhysicsSystem();
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
	JPH::TempAllocator& JoltManager::PhysicsAllocator()
	{
		return impl_->PhysicsAllocator();
	}

}
