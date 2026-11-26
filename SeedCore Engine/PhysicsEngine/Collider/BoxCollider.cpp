#include <PhysicsEngine/Collider/BoxCollider.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>

namespace SeedCore
{
	/**
	* [JP]
	* 当たり判定の体を作る。
	*
	* @brief 当たり判定を作る
	*
	* [EN]
	* Creates the collision body.
	*
	* @brief Creates the collision
	*/
	void BoxCollider::OnAwake()
	{
		shapeHandle_ = GetShapeHandle();
		bodyID_ = PhysicsSystem::CreateColliderBody(GetActor(), shapeHandle_, isTrigger_);

		/// [JP] 作ったときの大きさを覚えておく
		/// [EN] Remember the scale it was made at
		Matrix worldMatrix = GetActor().WorldMatrix();
		Quaternion worldRotation;
		Vector3 worldTranslation;
		worldMatrix.Decompose(worldTranslation, worldRotation, shapeScale_);
	}

	/**
	* [JP]
	* 当たり判定の体を消す。
	*
	* @brief 当たり判定を消す
	*
	* [EN]
	* Removes the collision body.
	*
	* @brief Removes the collision
	*/
	void BoxCollider::OnDestroy()
	{
		PhysicsSystem::DestroyColliderBody(GetActor(), bodyID_, shapeHandle_);
	}

	/**
	* [JP]
	* 設定が変わったので、当たり判定を作り直す。
	*
	* @brief 当たり判定を作り直す
	*
	* [EN]
	* The settings changed, so the collision is made again.
	*
	* @brief Remakes the collision
	*/
	void BoxCollider::OnRevision()
	{
		OnDestroy();
		OnAwake();
	}

	/**
	* [JP]
	* 当たり判定を作り直す。
	*
	* @brief 当たり判定を作り直す
	*
	* [EN]
	* Remakes the collision.
	*
	* @brief Remakes the collision
	*/
	void BoxCollider::Dispatch()
	{
		OnRevision();
	}

	/**
	* [JP]
	* 今の大きさに合わせた形を作って返す。
	*
	* @brief  形を作る
	* @return 作った形
	*
	* [EN]
	* Makes and returns a shape that fits the current size.
	*
	* @brief  Makes the shape
	* @return The new shape
	*/
	ScShapeHandle BoxCollider::GetShapeHandle()const
	{
		Actor actor = GetActor();
		const Scale* scale = actor.GetComponent<Scale>();

		/// [JP] 親の大きさもかけて、世界での大きさを出す
		/// [EN] Multiply in the parent's scale to get the world scale
		Actor parentActor = actor.Parent();
		Vector3 parentScale = Vector3::One;
		Quaternion parentRotation;
		Vector3 parentTranslation;
		if (parentActor)
		{
			Matrix parentMatrix = parentActor.WorldMatrix();
			parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);
		}
		Vector3 worldScale = Vector3(scale->x_, scale->y_, scale->z_) * parentScale;

		/// [JP] 大きさに合わせて形の寸法を決める
		/// [EN] Size the shape to match the scale
		Vector3 size(size_.x * Abs(worldScale.x), size_.y * Abs(worldScale.y), size_.z * Abs(worldScale.z));

		Vector3 center(center_.x * worldScale.x, center_.y * worldScale.y, center_.z * worldScale.z);

		return actor.GetService<Physics>()->CreateBoxShape(size, center);
	}

	/**
	* [JP]
	* アクターの位置と向きを当たり判定に伝える。
	* 大きさが変わっていたら形も作り直す。
	*
	* @brief 位置と大きさを合わせる
	*
	* [EN]
	* Sends the actor's position and facing to the collision.
	* If the size changed, the shape is made again too.
	*
	* @brief Matches the position and size
	*/
	void BoxCollider::Refresh()
	{
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		Actor actor = GetActor();
		Physics& physics = *actor.GetService<Physics>();

		/// [JP] 位置と向きを伝える
		/// [EN] Send the position and facing
		RigidbodyDesc desc;
		PhysicsSystem::ApplyTransform(actor, desc);
		physics.BodyTransform(bodyID_, desc.position_, desc.rotation_);

		/// [JP] 大きさが変わっていなければここで終わる
		/// [EN] Stop here if the scale has not changed
		Matrix worldMatrix = actor.WorldMatrix();
		Vector3 worldScale;
		Quaternion worldRotation;
		Vector3 worldTranslation;
		worldMatrix.Decompose(worldTranslation, worldRotation, worldScale);
		if (worldScale == shapeScale_)
		{
			return;
		}

		/// [JP] 形を作り直して入れ替える
		/// [EN] Remake the shape and swap it in
		ScShapeHandle shapeHandle = GetShapeHandle();
		physics.BodyShape(bodyID_, shapeHandle);
		physics.ReleaseShape(shapeHandle_);
		shapeHandle_ = shapeHandle;
		shapeScale_ = worldScale;
	}
}
