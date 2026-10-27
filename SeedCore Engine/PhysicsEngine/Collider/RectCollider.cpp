#include <PhysicsEngine/Collider/RectCollider.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the rectangle shape and collider body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 矩形状とコライダーボディを生成する。
	*/
	void RectCollider::OnAwake()
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
	void RectCollider::OnDestroy()
	{
		PhysicsSystem::DestroyColliderBody(GetActor(), bodyID_, shapeHandle_);
	}

	void RectCollider::OnRevision()
	{
		OnDestroy();
		OnAwake();
	}

	void RectCollider::Dispatch()
	{
		OnRevision();
	}

	/**
	* [EN]
	* Creates and returns a rectangle shape from the current settings,
	* scaled by the X and Y of the actor's own Scale combined with its
	* parent's scale.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の設定に Actor 自身の Scale と親のスケールの X と Y を掛けた矩形状を生成して
	* 返す。
	*/
	Handle<JPH::Shape> RectCollider::GetShapeHandle()const
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

		/// [EN] The rectangle lies in the canvas plane, so only X and Y apply; a mirrored axis keeps a positive size.
		/// [JP] 矩形は Canvas の平面上にあるので、効くのは X と Y だけ。反転した軸でもサイズは正のまま。
		Vector2 size(size_.x * Abs(worldScale.x), size_.y * Abs(worldScale.y));

		/// [EN] The offset is in local space, so it moves with the scale, sign included.
		/// [JP] オフセットはローカル空間なので、符号も含めてスケールに合わせて動く。
		Vector2 center(center_.x * worldScale.x, center_.y * worldScale.y);

		return actor.GetService<Physics>()->CreateRectShape(size, center);
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
	void RectCollider::Refresh()
	{
		/// [EN] With a Rigidbody present, the body and its pose belong to the Rigidbody.
		/// [JP] Rigidbody があれば、ボディとその姿勢は Rigidbody のもの。
		if (bodyID_.IsInvalid())
		{
			return;
		}

		Actor actor = GetActor();
		Physics& physics = *actor.GetService<Physics>();

		/// [EN] The pose is converted to the canvas body's space the same way as at creation; the body does not move when the pose is unchanged.
		/// [JP] 姿勢は生成時と同じ方法で Canvas のボディの空間へ変換する。姿勢が変わっていなければボディは動かない。
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
