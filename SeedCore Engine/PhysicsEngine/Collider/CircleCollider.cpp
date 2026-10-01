#include <PhysicsEngine/Collider/CircleCollider.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the circle shape and collider body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 円形状とコライダーボディを生成する。
	*/
	void CircleCollider::OnAwake()
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
	void CircleCollider::OnDestroy()
	{
		PhysicsSystem::DestroyColliderBody(GetActor(), bodyID_, shapeHandle_);
	}

	/**
	* [EN]
	* Creates and returns a circle shape from the current settings, scaled
	* by the X and Y of the actor's own Scale; the parent's scale is not
	* included. The radius follows the larger of X and Y.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の設定に Actor 自身の Scale の X と Y を掛けた円形状を生成して
	* 返す。親のスケールは含めない。半径は X と Y の大きい方に合わせる。
	*/
	Handle<JPH::Shape> CircleCollider::GetShapeHandle()const
	{
		Actor actor = GetActor();
		const Scale* scale = actor.GetComponent<Scale>();

		/// [EN] A circle cannot stretch per axis, so the radius takes the larger of the X and Y scales.
		/// [JP] 円は軸ごとには伸ばせないので、半径には X と Y の大きい方のスケールを使う。
		Float radius = radius_ * Max(Abs(scale->x_), Abs(scale->y_));

		/// [EN] The offset is in local space, so it moves with the scale, sign included.
		/// [JP] オフセットはローカル空間なので、符号も含めてスケールに合わせて動く。
		Vector2 center(center_.x * scale->x_, center_.y * scale->y_);

		return actor.GetPhysics().CreateCircleShape(radius, center);
	}
}
