#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	/**
	* [EN]
	* Component that creates a rectangular 2D collider body for its actor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アクター用の矩形 2D コライダーボディを生成するコンポーネント。
	*/
	class SEEDCORE_API RectCollider :public SeedScript
	{
	private:
		friend class PhysicsSystem;

	public:
		/// [EN] Full width and height of the rectangle.
		/// [JP] 矩形の幅と高さ。
		SC_REFLECTION_FIELD_EX("サイズ")
		Vector2 size_ = { 100.0f, 100.0f };

		/// [EN] Local offset of the rectangle center.
		/// [JP] 矩形中心のローカルオフセット。
		SC_REFLECTION_FIELD_EX("中心オフセット")
		Vector2 center_ = { 0.0f, 0.0f };

		/// [EN] Whether this collider acts as a trigger.
		/// [JP] このコライダーをトリガーとして扱うか。
		SC_REFLECTION_FIELD_EX("トリガー")
		Bool isTrigger_ = false;

	public:
		/**
		* [EN]
		* Creates the rectangle shape and collider body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 矩形状とコライダーボディを生成する。
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
		* Creates and returns a rectangle shape from the current settings,
		* scaled by the X and Y of the actor's own Scale combined with its
		* parent's scale.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在の設定に Actor 自身の Scale と親のスケールの X と Y を掛けた矩形状を
		* 生成して返す。
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
	REGISTER_COMPONENT(RectCollider, "Collider");
}
