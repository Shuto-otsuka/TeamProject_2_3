#include <PhysicsEngine/Collider/SphereCollider.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the sphere shape and collider body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 球形状とコライダーボディを生成する。
	*/
	void SphereCollider::OnAwake()
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
	void SphereCollider::OnDestroy()
	{
		PhysicsSystem::DestroyColliderBody(GetActor(), bodyID_, shapeHandle_);
	}

	/**
	* [EN]
	* Creates and returns a sphere shape from the current settings, scaled
	* by the actor's own Scale; the parent's scale is not included. The
	* radius follows the largest axis, so the sphere encloses the scaled
	* shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の設定に Actor 自身の Scale を掛けた球形状を生成して返す。
	* 親のスケールは含めない。半径は最も大きい軸に合わせるので、球は
	* 拡縮後の形を包む。
	*/
	Handle<JPH::Shape> SphereCollider::GetShapeHandle()const
	{
		Actor actor = GetActor();
		const Scale* scale = actor.GetComponent<Scale>();

		/// [EN] A sphere cannot stretch per axis, so the radius takes the largest scale; a mirrored axis counts by its size.
		/// [JP] 球は軸ごとには伸ばせないので、半径には最も大きいスケールを使う。反転した軸は大きさで数える。
		Float radius = radius_ * Max(Abs(scale->x_), Abs(scale->y_), Abs(scale->z_));

		return actor.GetPhysics().CreateSphereShape(radius);
	}
}
