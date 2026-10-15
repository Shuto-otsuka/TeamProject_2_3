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
	* by the X and Y of the actor's own Scale combined with its parent's
	* scale. The radius follows the larger of X and Y.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の設定に Actor 自身の Scale と親のスケールの X と Y を掛けた円形状を生成して
	* 返す。半径は X と Y の大きい方に合わせる。
	*/
	Handle<JPH::Shape> CircleCollider::GetShapeHandle()const
	{
		Actor actor = GetActor();
		const Scale* scale = actor.GetComponent<Scale>();

		/// [EN] The parent's accumulated scale is taken from its world matrix and multiplied in per axis, so the shape matches how the actor is drawn; a root actor's parent scale is 1.
		/// [JP] 親の累積スケールをそのワールド行列から取り出して軸ごとに掛け、描画される actor と形を合わせる。ルートの Actor の親スケールは 1。
		Actor parentActor = actor.Parent();
		Vector3 parentScale = Vector3::One;
		Quaternion parentRotation;
		Vector3 parentTranslation;
		if (parentActor)
		{
			Matrix parentMatrix = parentActor.WorldMatrix();
			parentMatrix.Decompose(parentScale, parentRotation, parentTranslation);
		}
		Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

		/// [EN] A circle cannot stretch per axis, so the radius takes the larger of the X and Y scales.
		/// [JP] 円は軸ごとには伸ばせないので、半径には X と Y の大きい方のスケールを使う。
		Float radius = radius_ * Max(Abs(worldScale.x), Abs(worldScale.y));

		/// [EN] The offset is in local space, so it moves with the scale, sign included.
		/// [JP] オフセットはローカル空間なので、符号も含めてスケールに合わせて動く。
		Vector2 center(center_.x * worldScale.x, center_.y * worldScale.y);

		return actor.GetPhysics().CreateCircleShape(radius, center);
	}
}
