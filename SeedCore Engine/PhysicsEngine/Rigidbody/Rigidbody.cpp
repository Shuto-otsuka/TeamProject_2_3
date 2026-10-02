#include <PhysicsEngine/Rigidbody/Rigidbody.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <PhysicsEngine/JoltPhysics/JoltLayerdef.h>
#include <PhysicsEngine/Collider/BoxCollider.h>
#include <PhysicsEngine/Collider/SphereCollider.h>
#include <PhysicsEngine/Collider/CapsuleCollider.h>
#include <PhysicsEngine/Collider/CylinderCollider.h>
#include <PhysicsEngine/Collider/RectCollider.h>
#include <PhysicsEngine/Collider/CircleCollider.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the body from the actor's collider shape, pose and this
	* component's settings, and adds it to the simulation.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Actor のコライダーの形状と姿勢、このコンポーネントの設定からボディを作り、
	* シミュレーションへ加える。
	*/
	void Rigidbody::OnAwake()
	{
		Actor actor = GetActor();

		/// [EN] Borrows the shape of the first collider found; each call adds a reference this component owns.
		/// [JP] 最初に見つかったコライダーの形状を借りる。呼ぶたびに、このコンポーネントが持つ参照が1つ増える。
		/// [EN] A MeshCollider is not looked for: its shape arrives later and replaces the fallback sphere.
		/// [JP] MeshCollider は探さない。その形状は後から届き、代わりの球と差し替わる。
		shapeHandle_ = [&]() -> Handle<JPH::Shape>
		{
			if (BoxCollider* collider = actor.GetComponent<BoxCollider>())
			{
				return collider->GetShapeHandle();
			}
			if (SphereCollider* collider = actor.GetComponent<SphereCollider>())
			{
				return collider->GetShapeHandle();
			}
			if (CapsuleCollider* collider = actor.GetComponent<CapsuleCollider>())
			{
				return collider->GetShapeHandle();
			}
			if (CylinderCollider* collider = actor.GetComponent<CylinderCollider>())
			{
				return collider->GetShapeHandle();
			}
			if (RectCollider* collider = actor.GetComponent<RectCollider>())
			{
				return collider->GetShapeHandle();
			}
			if (CircleCollider* collider = actor.GetComponent<CircleCollider>())
			{
				return collider->GetShapeHandle();
			}

			/// [EN] Without a collider the body still needs a shape, so a small sphere stands in.
			/// [JP] コライダーが無くてもボディには形状が要るので、小さな球で代用する。
			/// [EN] Like SphereCollider, the radius follows the largest axis of the actor's own Scale; the parent's scale is not included.
			/// [JP] SphereCollider と同じく、半径は Actor 自身の Scale の最も大きい軸に合わせる。親のスケールは含めない。
			const Scale* scale = actor.GetComponent<Scale>();
			return actor.GetPhysics().CreateSphereShape(defaultShapeRadius_ * Max(Abs(scale->x_), Abs(scale->y_), Abs(scale->z_)));
		}();

		/// [EN] Starts with every axis free and removes each frozen one.
		/// [JP] 全ての軸を自由にした状態から、固定した軸を1つずつ外す。
		JPH::EAllowedDOFs allowedDOFs = [&]() -> JPH::EAllowedDOFs
		{
			JPH::EAllowedDOFs dofs = JPH::EAllowedDOFs::All;
			if (freezePositionX_)
			{
				dofs &= ~JPH::EAllowedDOFs::TranslationX;
			}
			if (freezePositionY_)
			{
				dofs &= ~JPH::EAllowedDOFs::TranslationY;
			}
			if (freezePositionZ_)
			{
				dofs &= ~JPH::EAllowedDOFs::TranslationZ;
			}
			if (freezeRotationX_)
			{
				dofs &= ~JPH::EAllowedDOFs::RotationX;
			}
			if (freezeRotationY_)
			{
				dofs &= ~JPH::EAllowedDOFs::RotationY;
			}
			if (freezeRotationZ_)
			{
				dofs &= ~JPH::EAllowedDOFs::RotationZ;
			}

			return dofs;
		}();

		/// [EN] Shape, pose and EntityID, then the motion type and the layer packed from it and the actor's layer.
		/// [JP] 形状・姿勢・EntityID、続いて運動タイプと、それと Actor のレイヤーからパックしたレイヤー。
		RigidbodyDesc desc;
		desc.shape_ = shapeHandle_;
		PhysicsSystem::ApplyTransform(actor, desc);
		desc.motionType_ = ToMotionType(bodyType_);
		desc.continuousCollision_ = continuousCollision_;
		desc.layer_ = ToObjectLayer(bodyType_, actor.Layer());

		/// [EN] Material and motion settings; gravity off is a gravity factor of 0.
		/// [JP] 材質と運動の設定。重力を切るのは、重力の倍率を 0 にすることと同じ。
		desc.mass_ = mass_;
		desc.linearDamping_ = linearDrag_;
		desc.angularDamping_ = angularDrag_;
		desc.friction_ = friction_;
		desc.restitution_ = restitution_;
		desc.gravityFactor_ = useGravity_ ? gravityScale_ : 0.0f;
		desc.allowedDOFs_ = allowedDOFs;
		desc.isSensor_ = isTrigger_;

		/// [EN] A canvas body lives on the z = 0 plane and only collides with other canvas bodies.
		/// [JP] Canvas のボディは z = 0 の平面上にあり、他の Canvas のボディとだけ衝突する。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			/// [EN] Only X/Y movement and rotation about Z remain; the 3D freeze settings are remapped below.
			/// [JP] 残るのは X/Y の移動と Z 軸まわりの回転だけ。3D 用の固定設定は下で置き換える。
			desc.layer_ |= Layers::PLANAR;
			desc.allowedDOFs_ = JPH::EAllowedDOFs::Plane2D;

			if (freezePositionX_)
			{
				desc.allowedDOFs_ &= ~JPH::EAllowedDOFs::TranslationX;
			}

			if (freezePositionY_)
			{
				desc.allowedDOFs_ &= ~JPH::EAllowedDOFs::TranslationY;
			}

			/// [EN] The canvas keeps its in-plane angle in Rotation::x_, so the X freeze also locks it.
			/// [JP] Canvas は平面内の角度を Rotation::x_ に持つので、X の固定でもそれを固定する。
			if (freezeRotationX_ || freezeRotationZ_)
			{
				desc.allowedDOFs_ &= ~JPH::EAllowedDOFs::RotationZ;
			}
		}

		bodyID_ = actor.GetPhysics().CreateRigidbody(desc);
	}

	/**
	* [EN]
	* Copies the simulated pose of the body back to the actor's
	* Position and Rotation.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シミュレーション後のボディの姿勢を、Actor の Position と Rotation へ
	* 書き戻す。
	*/
	void Rigidbody::OnFixedTick(Float elapsedTime)
	{
		if (bodyID_.IsInvalid())
		{
			return;
		}

		Actor actor = GetActor();
		World& world = actor.GetWorld();
		Entity entity = actor.GetEntity();

		/// [EN] With neither Position nor Rotation there is nothing to write back.
		/// [JP] Position も Rotation も無ければ、書き戻す先が無い。
		Position* position = world.GetComponent<Position>(entity);
		Rotation* rotation = world.GetComponent<Rotation>(entity);
		if (!position && !rotation)
		{
			return;
		}

		Vector3 outPosition;
		Quaternion outRotation;
		actor.GetPhysics().BodyTransform(bodyID_, outPosition, outRotation);

		/// [EN] Canvas bodies go back from meters (Y up) to pixels (Y down).
		/// [JP] Canvas のボディは、メートル(Y 上向き)からピクセル(Y 下向き)へ戻す。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			if (position)
			{
				position->x_ = outPosition.x * pixelsPerMeter_;
				position->y_ = -outPosition.y * pixelsPerMeter_;
			}

			/// [EN] The body only turns about Z, so the angle is 2·atan2(z, w), negated with the flipped Y and stored as the X component of Rotation's Euler angles.
			/// [JP] ボディは Z 軸まわりにしか回らないので、角度は 2·atan2(z, w)。Y の反転に合わせて符号を反転し、Rotation のオイラー角の X 成分として格納する。
			if (rotation)
			{
				Quaternion canvasRotation = Quaternion::CreateFromAxisAngle(Vector3::UnitX, -2.0f * std::atan2(outRotation.z, outRotation.w));
				rotation->x_ = canvasRotation.x;
				rotation->y_ = canvasRotation.y;
				rotation->z_ = canvasRotation.z;
				rotation->w_ = canvasRotation.w;
			}

			return;
		}

		if (position)
		{
			position->x_ = outPosition.x;
			position->y_ = outPosition.y;
			position->z_ = outPosition.z;
		}

		/// [EN] Rotation is stored as a normalized quaternion.
		/// [JP] Rotation は正規化済みクォータニオンで持つ。
		if (rotation)
		{
			rotation->x_ = outRotation.x;
			rotation->y_ = outRotation.y;
			rotation->z_ = outRotation.z;
			rotation->w_ = outRotation.w;
		}
	}

	/**
	* [EN]
	* Destroys the body and releases the reference to its shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディを破棄し、形状への参照を解放する。
	*/
	void Rigidbody::OnDestroy()
	{
		if (bodyID_.IsInvalid())
		{
			return;
		}

		/// [EN] Only this component's reference is released; the collider keeps its own.
		/// [JP] 解放するのはこのコンポーネントの参照だけで、コライダーは自分の参照を持ち続ける。
		Actor actor = GetActor();
		actor.GetPhysics().DestroyBody(bodyID_);
		actor.GetPhysics().ReleaseShape(shapeHandle_);

		bodyID_ = JPH::BodyID();
	}

	/**
	* [EN]
	* Adds a force (N) at the center of mass for the next fixed step.
	* On a canvas body the force is given in pixels with Y down, and
	* its Z is ignored. Only a Dynamic body is affected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 次の固定ステップの間、重心に力(N)を加える。Canvas のボディでは
	* 力をピクセル単位・Y 下向きで与え、Z は無視する。効くのは Dynamic
	* のボディだけ。
	*/
	void Rigidbody::AddForce(const Vector3& force)
	{
		Actor actor = GetActor();

		/// [EN] Canvas bodies go from pixels (Y down) to meters (Y up); the body stays on the z = 0 plane.
		/// [JP] Canvas のボディは、ピクセル(Y 下向き)からメートル(Y 上向き)へ直す。ボディは z = 0 の平面上に留まる。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetPhysics().AddForce(bodyID_, Vector3(force.x / pixelsPerMeter_, -force.y / pixelsPerMeter_, 0.0f));
			return;
		}

		actor.GetPhysics().AddForce(bodyID_, force);
	}

	/**
	* [EN]
	* Adds an impulse (kg·m/s) at the center of mass, changing the
	* velocity at once. On a canvas body the impulse is given in pixels
	* with Y down, and its Z is ignored. Only a Dynamic body is affected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 重心に力積(kg·m/s)を加え、速度を一度に変える。Canvas のボディでは
	* 力積をピクセル単位・Y 下向きで与え、Z は無視する。効くのは Dynamic
	* のボディだけ。
	*/
	void Rigidbody::AddImpulse(const Vector3& impulse)
	{
		Actor actor = GetActor();

		/// [EN] Canvas bodies go from pixels (Y down) to meters (Y up); the body stays on the z = 0 plane.
		/// [JP] Canvas のボディは、ピクセル(Y 下向き)からメートル(Y 上向き)へ直す。ボディは z = 0 の平面上に留まる。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetPhysics().AddImpulse(bodyID_, Vector3(impulse.x / pixelsPerMeter_, -impulse.y / pixelsPerMeter_, 0.0f));
			return;
		}

		actor.GetPhysics().AddImpulse(bodyID_, impulse);
	}

	/**
	* [EN]
	* Adds a world-space torque (N·m) for the next fixed step. On a
	* canvas body only X is used, as the in-plane torque in the same
	* direction as Rotation::x_. Only a Dynamic body is affected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 次の固定ステップの間、ワールド空間のトルク(N·m)を加える。Canvas の
	* ボディでは X だけを、Rotation::x_ と同じ向きの平面内トルクとして使う。
	* 効くのは Dynamic のボディだけ。
	*/
	void Rigidbody::AddTorque(const Vector3& torque)
	{
		Actor actor = GetActor();

		/// [EN] A canvas body only turns about Z. Its angle is kept in Rotation::x_ with the sign flipped, so X is negated onto Z.
		/// [JP] Canvas のボディは Z 軸まわりにしか回らない。角度は符号を反転して Rotation::x_ に持つので、X を反転して Z へ移す。
		/// [EN] Torque carries length squared (kg·m²/s²), so pixels come back to meters by dividing twice.
		/// [JP] トルクは長さの2乗(kg·m²/s²)を含むので、ピクセルからメートルへは2回割って戻す。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetPhysics().AddTorque(bodyID_, Vector3(0.0f, 0.0f, -torque.x / (pixelsPerMeter_ * pixelsPerMeter_)));
			return;
		}

		actor.GetPhysics().AddTorque(bodyID_, torque);
	}

	/**
	* [EN]
	* Adds a world-space angular impulse (N·m·s), changing the angular
	* velocity at once. On a canvas body only X is used, as the
	* in-plane angular impulse in the same direction as Rotation::x_.
	* Only a Dynamic body is affected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワールド空間の角力積(N·m·s)を加え、角速度を一度に変える。Canvas の
	* ボディでは X だけを、Rotation::x_ と同じ向きの平面内の角力積として
	* 使う。効くのは Dynamic のボディだけ。
	*/
	void Rigidbody::AddSpin(const Vector3& angularImpulse)
	{
		Actor actor = GetActor();

		/// [EN] A canvas body only turns about Z. Its angle is kept in Rotation::x_ with the sign flipped, so X is negated onto Z.
		/// [JP] Canvas のボディは Z 軸まわりにしか回らない。角度は符号を反転して Rotation::x_ に持つので、X を反転して Z へ移す。
		/// [EN] Angular impulse carries length squared (kg·m²/s), so pixels come back to meters by dividing twice.
		/// [JP] 角力積は長さの2乗(kg·m²/s)を含むので、ピクセルからメートルへは2回割って戻す。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetPhysics().AddSpin(bodyID_, Vector3(0.0f, 0.0f, -angularImpulse.x / (pixelsPerMeter_ * pixelsPerMeter_)));
			return;
		}

		actor.GetPhysics().AddSpin(bodyID_, angularImpulse);
	}

	/**
	* [EN]
	* Returns the Jolt ID of the body; invalid before OnAwake and after
	* OnDestroy.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディの Jolt ID を返す。OnAwake 前と OnDestroy 後は無効な ID。
	*/
	JPH::BodyID Rigidbody::BodyID()const
	{
		return bodyID_;
	}
}
