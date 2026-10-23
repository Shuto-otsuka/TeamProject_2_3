#include <PhysicsEngine/Collider/SphereCollider.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <FoundationEngine/World/World.h>
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

		/// [EN] Remembers the world scale the shape was made for, so Refresh only rebuilds it once the scale actually changes.
		/// [JP] 形状を作ったときのワールドスケールを覚え、Refresh が実際にスケールが変わったときだけ作り直すようにする。
		Matrix worldMatrix = GetActor().WorldMatrix();
		Quaternion worldRotation;
		Vector3 worldTranslation;
		worldMatrix.Decompose(shapeScale_, worldRotation, worldTranslation);
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
	* by the actor's own Scale combined with its parent's scale. The
	* radius follows the largest axis, so the sphere encloses the scaled
	* shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の設定に Actor 自身の Scale と親のスケールを掛けた球形状を生成して返す。
	* 半径は最も大きい軸に合わせるので、球は
	* 拡縮後の形を包む。
	*/
	Handle<JPH::Shape> SphereCollider::GetShapeHandle()const
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

		/// [EN] A sphere cannot stretch per axis, so the radius takes the largest scale; a mirrored axis counts by its size.
		/// [JP] 球は軸ごとには伸ばせないので、半径には最も大きいスケールを使う。反転した軸は大きさで数える。
		Float radius = radius_ * Max(Abs(worldScale.x), Abs(worldScale.y), Abs(worldScale.z));

		return actor.GetService<Physics>()->CreateSphereShape(radius);
	}

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
	void SphereCollider::Refresh()
	{
		/// [EN] With a Rigidbody present, the body and its pose belong to the Rigidbody.
		/// [JP] Rigidbody があれば、ボディとその姿勢は Rigidbody のもの。
		if (bodyID_.IsInvalid())
		{
			return;
		}

		Actor actor = GetActor();
		Physics& physics = *actor.GetService<Physics>();

		/// [EN] The pose is worked out the same way as at creation; the body does not move when the pose is unchanged.
		/// [JP] 姿勢は生成時と同じ方法で求める。姿勢が変わっていなければボディは動かない。
		RigidbodyDesc desc;
		PhysicsSystem::ApplyTransform(actor, desc);
		physics.BodyTransform(bodyID_, desc.position_, desc.rotation_);

		/// [EN] The actor's world matrix carries its own scale combined with every ancestor's.
		/// [JP] Actor のワールド行列には、自身のスケールと全ての祖先のスケールを合わせたものが入っている。
		Matrix worldMatrix = actor.WorldMatrix();
		Vector3 worldScale;
		Quaternion worldRotation;
		Vector3 worldTranslation;
		worldMatrix.Decompose(worldScale, worldRotation, worldTranslation);
		if (worldScale == shapeScale_)
		{
			return;
		}

		/// [EN] The new shape is attached before the old one is released, so the body never points at a freed shape.
		/// [JP] 古い形状を解放する前に新しい形状を取り付け、ボディが解放済みの形状を指すことが無いようにする。
		Handle<JPH::Shape> shapeHandle = GetShapeHandle();
		physics.BodyShape(bodyID_, shapeHandle);
		physics.ReleaseShape(shapeHandle_);
		shapeHandle_ = shapeHandle;
		shapeScale_ = worldScale;
	}
}
