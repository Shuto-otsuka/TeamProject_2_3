#include <PhysicsEngine/Collider/CapsuleCollider.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the capsule shape and collider body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* カプセル形状とコライダーボディを生成する。
	*/
	void CapsuleCollider::OnAwake()
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
	void CapsuleCollider::OnDestroy()
	{
		PhysicsSystem::DestroyColliderBody(GetActor(), bodyID_, shapeHandle_);
	}

	/**
	* [EN]
	* Creates and returns a capsule shape from the current settings, scaled
	* by the actor's own Scale; the parent's scale is not included. The
	* height follows Y and the radius the larger of X and Z.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の設定に Actor 自身の Scale を掛けたカプセル形状を生成して返す。
	* 親のスケールは含めない。高さは Y に、半径は X と Z の大きい方に合わせる。
	*/
	Handle<JPH::Shape> CapsuleCollider::GetShapeHandle()const
	{
		Actor actor = GetActor();
		const Scale* scale = actor.GetComponent<Scale>();

		/// [EN] The capsule stands along Y, so its height stretches with the Y scale.
		/// [JP] カプセルは Y 軸に沿って立つので、高さは Y のスケールで伸びる。
		Float height = height_ * Abs(scale->y_);

		/// [EN] The cross-section must stay round, so the radius takes the larger of the X and Z scales.
		/// [JP] 断面は円のままでなければならないので、半径には X と Z の大きい方のスケールを使う。
		Float radius = radius_ * Max(Abs(scale->x_), Abs(scale->z_));

		return actor.GetPhysics().CreateCapsuleShape(height, radius);
	}
}
