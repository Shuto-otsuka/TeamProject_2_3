#include <PhysicsEngine/Collider/RectCollider.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the rectangle shape and collider body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 矩形状とコライダーボディを生成する。
	*/
	void RectCollider::OnAwake()
	{
		shapeHandle_ = GetShapeHandle();
		bodyID_ = PhysicsSystem::CreateColliderBody(GetActor(), shapeHandle_, isTrigger_);
	}

	/**
	* [EN]
	* Destroys the collider body and releases its shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コライダーボディを破棄し、形状を解放する。
	*/
	void RectCollider::OnDestroy()
	{
		PhysicsSystem::DestroyColliderBody(GetActor(), bodyID_, shapeHandle_);
	}

	/**
	* [EN]
	* Creates and returns a rectangle shape from the current settings,
	* scaled by the X and Y of the actor's own Scale; the parent's scale
	* is not included.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の設定に Actor 自身の Scale の X と Y を掛けた矩形状を生成して
	* 返す。親のスケールは含めない。
	*/
	Handle<JPH::Shape> RectCollider::GetShapeHandle()const
	{
		Actor actor = GetActor();
		const Scale* scale = actor.GetComponent<Scale>();

		/// [EN] The rectangle lies in the canvas plane, so only X and Y apply; a mirrored axis keeps a positive size.
		/// [JP] 矩形は Canvas の平面上にあるので、効くのは X と Y だけ。反転した軸でもサイズは正のまま。
		Vector2 size(size_.x * Abs(scale->x_), size_.y * Abs(scale->y_));

		/// [EN] The offset is in local space, so it moves with the scale, sign included.
		/// [JP] オフセットはローカル空間なので、符号も含めてスケールに合わせて動く。
		Vector2 center(center_.x * scale->x_, center_.y * scale->y_);

		return actor.GetPhysics().CreateRectShape(size, center);
	}
}
