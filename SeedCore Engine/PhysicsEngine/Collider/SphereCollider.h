#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	/**
	* [EN]
	* Component that creates a sphere-shaped collider body for its actor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アクター用の球型コライダーボディを生成するコンポーネント。
	*/
	class SEEDCORE_API SphereCollider :public SeedScript
	{
	private:
		friend class PhysicsSystem;

	public:
		/// [EN] Radius of the sphere.
		/// [JP] 球の半径。
		SC_REFLECTION_CLAMPED_EX("半径", 0.001f, 100.0f)
		Float radius_ = 0.5f;

		/// [EN] Whether this collider acts as a trigger.
		/// [JP] このコライダーをトリガーとして扱うか。
		SC_REFLECTION_FIELD_EX("トリガー")
		Bool isTrigger_ = false;

	public:
		/**
		* [EN]
		* Creates the sphere shape and collider body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 球形状とコライダーボディを生成する。
		*/
		void OnAwake();

		/**
		* [EN]
		* Destroys the collider body and releases its shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コライダーボディを破棄し、形状を解放する。
		*/
		void OnDestroy();

	public:
		/**
		* [EN]
		* Creates and returns a sphere shape from the current settings, scaled
		* by the actor's own Scale combined with its parent's scale. The
		* radius follows the largest axis, so the sphere encloses the scaled
		* shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在の設定に Actor 自身の Scale と親のスケールを掛けた球形状を生成して返す。
		* 半径は最も大きい軸に合わせるので、球は拡縮後の形を包む。
		*/
		Handle<JPH::Shape> GetShapeHandle()const;

	private:
		/**
		* [EN]
		* Moves the collider's own body to the actor's current pose, and
		* rebuilds its shape when the actor's world scale has changed since
		* the shape was made. Does nothing when a Rigidbody owns the body.
		* Called every frame by PhysicsSystem::ApplyCollider.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コライダー自身のボディを Actor の今の姿勢へ移し、形状を作ってから
		* Actor のワールドスケールが変わっていれば形状を作り直す。ボディを
		* Rigidbody が持っている場合は何もしない。
		* PhysicsSystem::ApplyCollider から毎フレーム呼ばれる。
		*/
		void Refresh();

	private:
		/// [EN] Jolt body identifier of the generated collider body.
		/// [JP] 生成したコライダーボディの Jolt ボディ ID。
		JPH::BodyID bodyID_;

		/// [EN] Pool handle of the attached shape.
		/// [JP] 取り付けた形状のプールハンドル。
		Handle<JPH::Shape> shapeHandle_;

		/// [EN] World scale of the actor when the current shape was made; a different scale means the shape no longer matches how the actor is drawn.
		/// [JP] 今の形状を作ったときの Actor のワールドスケール。これと異なれば、形状は描画される Actor と合わなくなっている。
		Vector3 shapeScale_ = { 0.0f, 0.0f, 0.0f };
	};
	REGISTER_COMPONENT(SphereCollider, "Collider");
}
