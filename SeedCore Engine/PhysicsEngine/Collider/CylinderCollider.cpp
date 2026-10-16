#include <PhysicsEngine/Collider/CylinderCollider.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the cylinder shape and collider body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 円柱形状とコライダーボディを生成する。
	*/
	void CylinderCollider::OnAwake()
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
	void CylinderCollider::OnDestroy()
	{
		PhysicsSystem::DestroyColliderBody(GetActor(), bodyID_, shapeHandle_);
	}

	/**
	* [EN]
	* Creates and returns a cylinder shape from the current settings, scaled
	* by the actor's own Scale combined with its parent's scale. The
	* height follows Y and the radius the larger of X and Z.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の設定に Actor 自身の Scale と親のスケールを掛けた円柱形状を生成して返す。
	* 高さは Y に、半径は X と Z の大きい方に合わせる。
	*/
	Handle<JPH::Shape> CylinderCollider::GetShapeHandle()const
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

		/// [EN] The cylinder stands along Y, so its height stretches with the Y scale.
		/// [JP] 円柱は Y 軸に沿って立つので、高さは Y のスケールで伸びる。
		Float height = height_ * Abs(worldScale.y);

		/// [EN] The cross-section must stay round, so the radius takes the larger of the X and Z scales.
		/// [JP] 断面は円のままでなければならないので、半径には X と Z の大きい方のスケールを使う。
		Float radius = radius_ * Max(Abs(worldScale.x), Abs(worldScale.z));

		return actor.GetPhysics().CreateCylinderShape(height, radius);
	}
}
