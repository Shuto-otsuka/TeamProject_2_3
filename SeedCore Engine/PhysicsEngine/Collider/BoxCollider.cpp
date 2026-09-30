#include <PhysicsEngine/Collider/BoxCollider.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the box shape and collider body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 箱形状とコライダーボディを生成する。
	*/
	void BoxCollider::OnAwake()
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
	void BoxCollider::OnDestroy()
	{
		PhysicsSystem::DestroyColliderBody(GetActor(), bodyID_, shapeHandle_);
	}

	/**
	* [EN]
	* Creates and returns a box shape from the current settings, scaled
	* by the actor's own Scale; the parent's scale is not included.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の設定に Actor 自身の Scale を掛けた箱形状を生成して返す。
	* 親のスケールは含めない。
	*/
	Handle<JPH::Shape> BoxCollider::GetShapeHandle()const
	{
		Actor actor = GetActor();
		const Scale* scale = actor.GetComponent<Scale>();

		/// [EN] Each axis of the box stretches by the scale on that axis; a mirrored axis keeps a positive size.
		/// [JP] 箱の各軸は、その軸のスケールだけ伸びる。反転した軸でもサイズは正のまま。
		Vector3 size(size_.x * Abs(scale->x_), size_.y * Abs(scale->y_), size_.z * Abs(scale->z_));

		/// [EN] The offset is in local space, so it moves with the scale, sign included.
		/// [JP] オフセットはローカル空間なので、符号も含めてスケールに合わせて動く。
		Vector3 center(center_.x * scale->x_, center_.y * scale->y_, center_.z * scale->z_);

		return actor.GetPhysics().CreateBoxShape(size, center);
	}
}
