#include <PhysicsEngine/Collider/CylinderCollider.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <FoundationEngine/World/World.h>
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
	void CylinderCollider::OnDestroy()
	{
		PhysicsSystem::DestroyColliderBody(GetActor(), bodyID_, shapeHandle_);
	}

	void CylinderCollider::OnRevision()
	{
		OnDestroy();
		OnAwake();
	}

	void CylinderCollider::Dispatch()
	{
		OnRevision();
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
	ScShapeHandle CylinderCollider::GetShapeHandle()const
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

		return actor.GetService<Physics>()->CreateCylinderShape(height, radius);
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
	void CylinderCollider::Refresh()
	{
		/// [EN] With a Rigidbody present, the body and its pose belong to the Rigidbody.
		/// [JP] Rigidbody があれば、ボディとその姿勢は Rigidbody のもの。
		if (bodyID_ == ScInvalidBodyID)
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
		ScShapeHandle shapeHandle = GetShapeHandle();
		physics.BodyShape(bodyID_, shapeHandle);
		physics.ReleaseShape(shapeHandle_);
		shapeHandle_ = shapeHandle;
		shapeScale_ = worldScale;
	}
}
