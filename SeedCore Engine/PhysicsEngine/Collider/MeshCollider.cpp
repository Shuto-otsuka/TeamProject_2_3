#include <PhysicsEngine/Collider/MeshCollider.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <PhysicsEngine/Rigidbody/Rigidbody.h>
#include <GraphicsEngine/Model/Collision/MeshCollision.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>

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
	void MeshCollider::OnAwake()
	{
		pending_ = meshID_ != 0;
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
	void MeshCollider::OnDestroy()
	{
		if (shapeHandle_.empty())
		{
			return;
		}

		PhysicsSystem::DestroyColliderBody(GetActor(), bodyID_, shapeHandle_);

		bodyID_ = ScInvalidBodyID;
		shapeHandle_ = ScShapeHandle::null();
		pending_ = meshID_ != 0;
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
	void MeshCollider::OnRevision()
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
	void MeshCollider::Dispatch()
	{
		OnRevision();
	}

	/**
	* [JP]
	* 読み込んだメッシュから形を作って、当たり判定にする。
	*
	* @brief メッシュから当たり判定を作る
	* @param meshCollision 形に使うメッシュ
	* @note  動く Rigidbody では凸包になる
	*
	* [EN]
	* Makes a shape from the loaded mesh and uses it for collision.
	*
	* @brief Makes the collision from a mesh
	* @param meshCollision The mesh to build the shape from
	* @note  A moving Rigidbody always gets a convex shape
	*/
	void MeshCollider::Build(const MeshCollision& meshCollision)
	{
		Actor actor = GetActor();
		Rigidbody* rigidbody = actor.GetComponent<Rigidbody>();

		if (rigidbody && rigidbody->BodyID() == ScInvalidBodyID)
		{
			return;
		}

		/// [JP] 動く Rigidbody には凹んだ形が使えないので凸包にする
		/// [EN] A moving Rigidbody cannot use a dented shape, so make it convex
		Bool convex = convex_;
		if (!convex && rigidbody && rigidbody->bodyType_ == Rigidbody::BodyType::Dynamic)
		{
			SC_LOG_WARNING("MeshCollider: Dynamic な Rigidbody には凹メッシュ形状を使えないため凸包で生成します (assetID: {})", meshID_);
			convex = true;
		}

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
		Vector3 scaleVector = Transform::Vector(*scale) * parentScale;

		/// [JP] 凸包かメッシュのまま形を作る
		/// [EN] Make the shape, convex or as the mesh
		shapeHandle_ = convex ? actor.GetService<Physics>()->CreateConvexShape(meshID_, meshCollision.Positions(), scaleVector) : actor.GetService<Physics>()->CreateMeshShape(meshID_, meshCollision.Positions(), meshCollision.Indices(), scaleVector);

		if (shapeHandle_.empty())
		{
			return;
		}

		pending_ = false;

		/// [JP] Rigidbody があればそれに形をつけ、なければ当たり判定だけの体を作る
		/// [EN] Give the shape to the Rigidbody if there is one, otherwise make a collision-only body
		if (rigidbody)
		{
			actor.GetService<Physics>()->BodyShape(rigidbody->BodyID(), shapeHandle_);
			return;
		}

		bodyID_ = PhysicsSystem::CreateColliderBody(actor, shapeHandle_, isTrigger_);
	}

	/**
	* [JP]
	* まだ形を作れていないかを返す。
	*
	* @brief  形待ちか調べる
	* @return まだなら true
	*
	* [EN]
	* Returns whether the shape still needs to be made.
	*
	* @brief  Checks if the shape is pending
	* @return True if it is still pending
	*/
	Bool MeshCollider::Pending()const
	{
		return pending_;
	}
}
