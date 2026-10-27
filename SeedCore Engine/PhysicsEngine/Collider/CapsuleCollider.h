#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/SeedScript.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	/**
	* [EN]
	* Component that creates a capsule-shaped collider body for its actor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アクター用のカプセル型コライダーボディを生成するコンポーネント。
	*/
	class SEEDCORE_API CapsuleCollider :public SeedScript
	{
	private:
		friend class PhysicsSystem;

	public:
		/// [EN] Height of the cylinder part between the two hemispherical caps; the whole capsule is this plus twice the radius.
		/// [JP] 両端の半球にはさまれた円柱部分の高さ。カプセル全体の高さは、これに半径の2倍を足したもの。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("円柱の高さ", 0.001f, 100.0f)
		Float height_ = 2.0f;

		/// [EN] Radius of the capsule.
		/// [JP] カプセルの半径。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("半径", 0.001f, 100.0f)
		Float radius_ = 0.5f;

		/// [EN] Whether the collider reports overlaps without physical response.
		/// [JP] 物理応答を行わず重なりを通知するトリガーとして扱うか。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("トリガー")
		Bool isTrigger_ = false;

	public:
		/**
		* [EN]
		* Creates the capsule shape and collider body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* カプセル形状とコライダーボディを生成する。
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

		void OnRevision();

	public:
		void Dispatch();

	public:
		/**
		* [EN]
		* Creates and returns a capsule shape from the current settings, scaled
		* by the actor's own Scale combined with its parent's scale. The
		* height follows Y and the radius the larger of X and Z.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在の設定に Actor 自身の Scale と親のスケールを掛けたカプセル形状を生成して返す。
		* 高さは Y に、半径は X と Z の大きい方に合わせる。
		*/
		ScShapeHandle GetShapeHandle()const;

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
		ScBodyID bodyID_ = ScInvalidBodyID;

		/// [EN] Pool handle of the shape attached to the collider body.
		/// [JP] コライダーボディへ取り付けた形状のプールハンドル。
		ScShapeHandle shapeHandle_;

		/// [EN] World scale of the actor when the current shape was made; a different scale means the shape no longer matches how the actor is drawn.
		/// [JP] 今の形状を作ったときの Actor のワールドスケール。これと異なれば、形状は描画される Actor と合わなくなっている。
		Vector3 shapeScale_ = { 0.0f, 0.0f, 0.0f };
	};
	REGISTER_COMPONENT(CapsuleCollider, "Physics/Collider");
}
