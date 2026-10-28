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
		/**
		* [EN]
		* Creates an uninitialized Jolt manager.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 未初期化の Jolt マネージャーを生成する。
		*/
		JoltManager();

		/**
		* [EN]
		* Destroys the manager after explicit finalization. Defined in the
		* .cpp, where Impl is a complete type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 明示的な終了処理後にマネージャーを破棄する。Impl が完全型になる
		* .cpp 側で定義する。
		*/
		~JoltManager();

		/// [EN] Jolt and the Gateway hold pointers into the manager, so it is neither copied nor reassigned.
		/// [JP] Jolt と Gateway がマネージャーの中を指すポインタを持つので、コピーも代入もしない。
		JoltManager(const JoltManager&) = delete;
		JoltManager& operator=(const JoltManager&) = delete;

	public:
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
		Bool Initialize(JobExecutor& executor);

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
		class Impl;

		/// [EN] The Jolt backend itself, owned through a pointer so Jolt's headers stay out of this one.
		/// [JP] Jolt のバックエンド本体。Jolt のヘッダをこのヘッダから外すため、ポインタ越しに所有する。
		ResourcePtr<Impl> impl_;
	};
}
