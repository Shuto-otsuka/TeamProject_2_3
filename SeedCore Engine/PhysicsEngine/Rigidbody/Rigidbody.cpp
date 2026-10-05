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
#include <FoundationEngine/Log/Error.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>

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

			/// [EN] The canvas angle is the rotation about Z, so the Z freeze locks it.
			/// [JP] Canvas の角度は Z 軸まわりの回転なので、Z の固定でそれを固定する。
			if (freezeRotationZ_)
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

			/// [EN] The body only turns about Z, so the angle is 2·atan2(z, w), negated with the flipped Y and stored as a rotation about Z.
			/// [JP] ボディは Z 軸まわりにしか回らないので、角度は 2·atan2(z, w)。Y の反転に合わせて符号を反転し、Z 軸まわりの回転として格納する。
			if (rotation)
			{
				Quaternion canvasRotation = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, -2.0f * std::atan2(outRotation.z, outRotation.w));
				Transform::Quat(*rotation, canvasRotation);
			}

			return;
		}

		if (position)
		{
			Transform::Vector(*position, outPosition);
		}

		/// [EN] Rotation is stored as a normalized quaternion.
		/// [JP] Rotation は正規化済みクォータニオンで持つ。
		if (rotation)
		{
			Transform::Quat(*rotation, outRotation);
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
		/// [EN] The body exists only between OnAwake and OnDestroy, and a force moves only a Dynamic body.
		/// [JP] ボディが存在するのは OnAwake から OnDestroy までの間だけで、力で動くのは Dynamic のボディだけ。
		if (bodyID_.IsInvalid())
		{
			SC_LOG_ERROR("Rigidbody::AddForce を適用できません: ボディが生成されていません。");
			return;
		}

		if (bodyType_ != BodyType::Dynamic)
		{
			SC_LOG_ERROR("Rigidbody::AddForce を適用できません: Dynamic のボディにしか効きません。");
			return;
		}

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
		/// [EN] The body exists only between OnAwake and OnDestroy, and an impulse moves only a Dynamic body.
		/// [JP] ボディが存在するのは OnAwake から OnDestroy までの間だけで、力積で動くのは Dynamic のボディだけ。
		if (bodyID_.IsInvalid())
		{
			SC_LOG_ERROR("Rigidbody::AddImpulse を適用できません: ボディが生成されていません。");
			return;
		}

		if (bodyType_ != BodyType::Dynamic)
		{
			SC_LOG_ERROR("Rigidbody::AddImpulse を適用できません: Dynamic のボディにしか効きません。");
			return;
		}

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
	* canvas body only Z is used, as the in-plane torque in the same
	* direction as the canvas angle. Only a Dynamic body is affected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 次の固定ステップの間、ワールド空間のトルク(N·m)を加える。Canvas の
	* ボディでは Z だけを、Canvas の角度と同じ向きの平面内トルクとして使う。
	* 効くのは Dynamic のボディだけ。
	*/
	void Rigidbody::AddTorque(const Vector3& torque)
	{
		/// [EN] The body exists only between OnAwake and OnDestroy, and a torque turns only a Dynamic body.
		/// [JP] ボディが存在するのは OnAwake から OnDestroy までの間だけで、トルクで回るのは Dynamic のボディだけ。
		if (bodyID_.IsInvalid())
		{
			SC_LOG_ERROR("Rigidbody::AddTorque を適用できません: ボディが生成されていません。");
			return;
		}

		if (bodyType_ != BodyType::Dynamic)
		{
			SC_LOG_ERROR("Rigidbody::AddTorque を適用できません: Dynamic のボディにしか効きません。");
			return;
		}

		Actor actor = GetActor();

		/// [EN] A canvas body only turns about Z, and the canvas angle runs opposite to the physics angle because of the flipped Y, so Z is negated.
		/// [JP] Canvas のボディは Z 軸まわりにしか回らず、Y の反転により Canvas の角度は物理の角度と逆向きなので、Z を反転する。
		/// [EN] Torque carries length squared (kg·m²/s²), so pixels come back to meters by dividing twice.
		/// [JP] トルクは長さの2乗(kg·m²/s²)を含むので、ピクセルからメートルへは2回割って戻す。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetPhysics().AddTorque(bodyID_, Vector3(0.0f, 0.0f, -torque.z / (pixelsPerMeter_ * pixelsPerMeter_)));
			return;
		}

		actor.GetPhysics().AddTorque(bodyID_, torque);
	}

	/**
	* [EN]
	* Adds a world-space angular impulse (N·m·s), changing the angular
	* velocity at once. On a canvas body only Z is used, as the
	* in-plane angular impulse in the same direction as the canvas angle.
	* Only a Dynamic body is affected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワールド空間の角力積(N·m·s)を加え、角速度を一度に変える。Canvas の
	* ボディでは Z だけを、Canvas の角度と同じ向きの平面内の角力積として
	* 使う。効くのは Dynamic のボディだけ。
	*/
	void Rigidbody::AddSpin(const Vector3& angularImpulse)
	{
		/// [EN] The body exists only between OnAwake and OnDestroy, and an angular impulse turns only a Dynamic body.
		/// [JP] ボディが存在するのは OnAwake から OnDestroy までの間だけで、角力積で回るのは Dynamic のボディだけ。
		if (bodyID_.IsInvalid())
		{
			SC_LOG_ERROR("Rigidbody::AddSpin を適用できません: ボディが生成されていません。");
			return;
		}

		if (bodyType_ != BodyType::Dynamic)
		{
			SC_LOG_ERROR("Rigidbody::AddSpin を適用できません: Dynamic のボディにしか効きません。");
			return;
		}

		Actor actor = GetActor();

		/// [EN] A canvas body only turns about Z, and the canvas angle runs opposite to the physics angle because of the flipped Y, so Z is negated.
		/// [JP] Canvas のボディは Z 軸まわりにしか回らず、Y の反転により Canvas の角度は物理の角度と逆向きなので、Z を反転する。
		/// [EN] Angular impulse carries length squared (kg·m²/s), so pixels come back to meters by dividing twice.
		/// [JP] 角力積は長さの2乗(kg·m²/s)を含むので、ピクセルからメートルへは2回割って戻す。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetPhysics().AddSpin(bodyID_, Vector3(0.0f, 0.0f, -angularImpulse.z / (pixelsPerMeter_ * pixelsPerMeter_)));
			return;
		}

		actor.GetPhysics().AddSpin(bodyID_, angularImpulse);
	}

	/**
	* [EN]
	* Moves the body so that it reaches the target position and rotation
	* after elapsedTime, carrying along whatever stands on it. Call it
	* every fixed step with that step's elapsedTime. On a canvas body the
	* position is given in pixels with Y down and its Z is ignored, and
	* only the rotation about Z is used. Only a Kinematic body is affected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* elapsedTime 後に目標の位置と回転へ着くようボディを動かし、上に乗って
	* いるものも一緒に運ぶ。固定ステップごとに、そのステップの elapsedTime
	* を渡して呼ぶ。Canvas のボディでは位置をピクセル単位・Y 下向きで与えて
	* Z は無視し、回転は Z 軸まわりだけを使う。効くのは Kinematic のボディ
	* だけ。
	*/
	void Rigidbody::MoveTarget(const Vector3& targetPosition, const Quaternion& targetRotation, Float elapsedTime)
	{
		/// [EN] The body exists only between OnAwake and OnDestroy, and only a Kinematic body follows a target.
		/// [JP] ボディが存在するのは OnAwake から OnDestroy までの間だけで、目標に従うのは Kinematic のボディだけ。
		if (bodyID_.IsInvalid())
		{
			SC_LOG_ERROR("Rigidbody::MoveTarget を適用できません: ボディが生成されていません。");
			return;
		}

		if (bodyType_ != BodyType::Kinematic)
		{
			SC_LOG_ERROR("Rigidbody::MoveTarget を適用できません: Kinematic のボディにしか効きません。");
			return;
		}

		/// [EN] The body's velocity is the distance divided by elapsedTime, so the step must have a positive length.
		/// [JP] ボディの速度は距離を elapsedTime で割ったものなので、ステップの長さは正でなければならない。
		if (elapsedTime <= 0.0f)
		{
			SC_LOG_ERROR("Rigidbody::MoveTarget を適用できません: elapsedTime は正の値が必要です (現在: {})。", elapsedTime);
			return;
		}

		Actor actor = GetActor();

		/// [EN] A canvas body goes from pixels (Y down) to meters (Y up) on the z = 0 plane.
		/// [JP] Canvas のボディは、ピクセル(Y 下向き)から z = 0 平面上のメートル(Y 上向き)へ変換する。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			/// [EN] The canvas angle is 2·atan2(z, w); it is negated with the flipped Y and rebuilt as a pure rotation about Z.
			/// [JP] Canvas の角度は 2·atan2(z, w)。Y の反転に合わせて符号を反転し、Z 軸まわりだけの回転として作り直す。
			Quaternion physicsRotation = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, -2.0f * std::atan2(targetRotation.z, targetRotation.w));
			actor.GetPhysics().MoveTarget(bodyID_, Vector3(targetPosition.x / pixelsPerMeter_, -targetPosition.y / pixelsPerMeter_, 0.0f), physicsRotation, elapsedTime);
			return;
		}

		actor.GetPhysics().MoveTarget(bodyID_, targetPosition, targetRotation, elapsedTime);
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
