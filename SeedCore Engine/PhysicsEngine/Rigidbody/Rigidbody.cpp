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
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>
#include <FoundationEngine/World/ECS/Component/Velocity.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	/**
	* [JP]
	* つけたときに、速さの部品も一緒につける。
	*
	* @brief 速さの部品をつける
	*
	* [EN]
	* When attached, it also adds the velocity parts.
	*
	* @brief Adds the velocity parts
	*/
	void Rigidbody::OnAttach()
	{
		Actor actor = GetActor();
		actor.AddComponent<LinearVelocity>();
		actor.AddComponent<AngularVelocity>();
	}

	/**
	* [JP]
	* 外したときに、速さの部品も一緒に外す。
	*
	* @brief 速さの部品を外す
	*
	* [EN]
	* When removed, it also removes the velocity parts.
	*
	* @brief Removes the velocity parts
	*/
	void Rigidbody::OnDetach()
	{
		Actor actor = GetActor();
		actor.RemoveComponent<LinearVelocity>();
		actor.RemoveComponent<AngularVelocity>();
	}

	/**
	* [JP]
	* 設定から物理の体を作る。
	*
	* @brief 体を作る
	*
	* [EN]
	* Creates the physics body from the settings.
	*
	* @brief Creates the body
	*/
	void Rigidbody::OnAwake()
	{
		Actor actor = GetActor();

		/// [JP] 同じアクターのコライダーから形を取る
		/// [EN] Take the shape from a collider on the same actor
		shapeHandle_ = [&]() -> ScShapeHandle
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

			/// [JP] コライダーがなければ、大きさに合わせた球を作る
			/// [EN] With no collider, make a sphere that fits the scale
			const Scale* scale = actor.GetComponent<Scale>();
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
			return actor.GetService<Physics>()->CreateSphereShape(defaultShapeRadius_ * Max(Abs(worldScale.x), Abs(worldScale.y), Abs(worldScale.z)));
		}();

		/// [JP] 固定する向きを、動いてよい向きから外す
		/// [EN] Remove the locked directions from the allowed ones
		ScAllowedDOFs allowedDOFs = [&]() -> ScAllowedDOFs
		{
			ScAllowedDOFs dofs = ScAllowedDOFsAll;
			if (freezePositionX_)
			{
				dofs &= ~ScAllowedDOFsTranslationX;
			}
			if (freezePositionY_)
			{
				dofs &= ~ScAllowedDOFsTranslationY;
			}
			if (freezePositionZ_)
			{
				dofs &= ~ScAllowedDOFsTranslationZ;
			}
			if (freezeRotationX_)
			{
				dofs &= ~ScAllowedDOFsRotationX;
			}
			if (freezeRotationY_)
			{
				dofs &= ~ScAllowedDOFsRotationY;
			}
			if (freezeRotationZ_)
			{
				dofs &= ~ScAllowedDOFsRotationZ;
			}

			return dofs;
		}();

		/// [JP] 設定をまとめる。動き方はあとで変えられるようにしておく
		/// [EN] Gather the settings, and allow the motion type to change later
		RigidbodyDesc desc;
		desc.shape_ = shapeHandle_;
		PhysicsSystem::ApplyTransform(actor, desc);
		desc.motionType_ = ToMotionType(bodyType_);
		desc.allowMotionTypeChange_ = true;
		desc.continuousCollision_ = continuousCollision_;
		desc.layer_ = ToObjectLayer(bodyType_, actor.Layer());

		desc.mass_ = mass_;
		desc.linearDamping_ = linearDrag_;
		desc.angularDamping_ = angularDrag_;
		desc.friction_ = friction_;
		desc.restitution_ = restitution_;
		desc.gravityFactor_ = useGravity_ ? gravityScale_ : 0.0f;
		desc.allowedDOFs_ = allowedDOFs;
		desc.isSensor_ = isTrigger_;

		/// [JP] 重なり用なら層に印をつける
		/// [EN] Mark the layer for an overlap-only body
		if (isTrigger_)
		{
			desc.layer_ |= Layers::SENSOR;
		}

		/// [JP] 2D なら平面だけで動くようにする
		/// [EN] For 2D, keep it moving on the plane only
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			desc.layer_ |= Layers::PLANAR;
			desc.allowedDOFs_ = ScAllowedDOFsPlane2D;

			if (freezePositionX_)
			{
				desc.allowedDOFs_ &= ~ScAllowedDOFsTranslationX;
			}

			if (freezePositionY_)
			{
				desc.allowedDOFs_ &= ~ScAllowedDOFsTranslationY;
			}

			if (freezeRotationZ_)
			{
				desc.allowedDOFs_ &= ~ScAllowedDOFsRotationZ;
			}
		}

		/// [JP] 体を作って、今の速さを伝える
		/// [EN] Create the body and send the current speed
		bodyID_ = actor.GetService<Physics>()->CreateRigidbody(desc);

		ApplyLinearVelocity();
		ApplyAngularVelocity();
	}

	/**
	* [JP]
	* 物理の体の動きを、アクターの位置・向き・速さに写す。
	*
	* @brief 体の動きを写す
	* @param elapsedTime 前の物理の更新からの時間
	*
	* [EN]
	* Copies the physics body's motion into the actor's position, facing and velocity.
	*
	* @brief Copies the body's motion
	* @param elapsedTime Time since the last physics update
	*/
	void Rigidbody::OnFixedTick(Float elapsedTime)
	{
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		/// [JP] 足場から受け取った速さも、空気の抵抗で弱めていく
		/// [EN] Let the speed taken from the foothold fade with drag too
		if (followFoothold_ && bodyType_ == BodyType::Dynamic && !isTrigger_)
		{
			inheritedLinearVelocity_ *= Max(0.0f, 1.0f - linearDrag_ * elapsedTime);
			inheritedAngularVelocity_ *= Max(0.0f, 1.0f - angularDrag_ * elapsedTime);
		}

		Actor actor = GetActor();

		/// [JP] 体の速さから足場の分を引いて、速さの部品に写す。2D はピクセルに直す
		/// [EN] Copy the body speed minus the foothold part into the velocity part; pixels for 2D
		if (LinearVelocity* velocity = actor.GetComponent<LinearVelocity>())
		{
			Vector3 bodyVelocity = actor.GetService<Physics>()->BodyLinearVelocity(bodyID_) - inheritedLinearVelocity_;

			if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
			{
				syncedLinearVelocity_ = Vector3(bodyVelocity.x * pixelsPerMeter_, -bodyVelocity.y * pixelsPerMeter_, 0.0f);
			}
			else
			{
				syncedLinearVelocity_ = bodyVelocity;
			}

			velocity->x_ = syncedLinearVelocity_.x;
			velocity->y_ = syncedLinearVelocity_.y;
			velocity->z_ = syncedLinearVelocity_.z;
		}

		/// [JP] 回る速さも同じように写す
		/// [EN] Copy the spin the same way
		if (AngularVelocity* velocity = actor.GetComponent<AngularVelocity>())
		{
			Vector3 bodyAngularVelocity = actor.GetService<Physics>()->BodyAngularVelocity(bodyID_) - inheritedAngularVelocity_;

			if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
			{
				syncedAngularVelocity_ = Vector3(0.0f, 0.0f, -bodyAngularVelocity.z);
			}
			else
			{
				syncedAngularVelocity_ = bodyAngularVelocity;
			}

			velocity->x_ = syncedAngularVelocity_.x;
			velocity->y_ = syncedAngularVelocity_.y;
			velocity->z_ = syncedAngularVelocity_.z;
		}

		Position* position = actor.GetComponent<Position>();
		Rotation* rotation = actor.GetComponent<Rotation>();
		if (!position && !rotation)
		{
			return;
		}

		/// [JP] 体の位置と向きを取り、親から見た位置と向きに直す
		/// [EN] Read the body's position and facing and make them relative to the parent
		Matrix bodyMatrix = actor.GetService<Physics>()->BodyTransform(bodyID_);
		Vector3 outPosition = bodyMatrix.Translation();
		Quaternion outRotation = Quaternion::CreateFromRotationMatrix(bodyMatrix);

		Actor parentActor = actor.Parent();
		Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
		Vector3 parentScale;
		Quaternion parentRotation;
		Vector3 parentTranslation;
		parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);

		Matrix inverseParentMatrix = parentMatrix.Invert();
		Quaternion inverseParentRotation;
		inverseParentRotation = parentRotation.Inverse();

		/// [JP] 2D はピクセルに戻し、Z の回転だけを使う
		/// [EN] For 2D, turn back into pixels and use only the Z turn
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			Quaternion canvasRotation = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, -2.0f * std::atan2(outRotation.z, outRotation.w));

			if (position)
			{
				Vector3 localPosition = Vector3::TransformPosition(Vector3(outPosition.x * pixelsPerMeter_, -outPosition.y * pixelsPerMeter_, 0.0f), inverseParentMatrix);
				position->x_ = localPosition.x;
				position->y_ = localPosition.y;
			}

			if (rotation)
			{
				Transform::Quat(*rotation, canvasRotation * inverseParentRotation);
			}

			return;
		}

		/// [JP] 3D はそのまま書き込む
		/// [EN] For 3D, write them as they are
		if (position)
		{
			Transform::Vector(*position, Vector3::TransformPosition(outPosition, inverseParentMatrix));
		}

		if (rotation)
		{
			Transform::Quat(*rotation, outRotation * inverseParentRotation);
		}
	}

	/**
	* [JP]
	* 物理の体を消す。
	*
	* @brief 体を消す
	*
	* [EN]
	* Removes the physics body.
	*
	* @brief Removes the body
	*/
	void Rigidbody::OnDestroy()
	{
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		/// [JP] 体と形を消して、足場の分の速さも消す
		/// [EN] Remove the body and shape, and clear the foothold speed
		Actor actor = GetActor();
		actor.GetService<Physics>()->DestroyBody(bodyID_);
		actor.GetService<Physics>()->ReleaseShape(shapeHandle_);

		bodyID_ = ScInvalidBodyID;

		inheritedLinearVelocity_ = Vector3::Zero;
		inheritedAngularVelocity_ = Vector3::Zero;
	}

	/**
	* [JP]
	* 設定が変わったので、体を作り直す。
	*
	* @brief 体を作り直す
	*
	* [EN]
	* The settings changed, so the body is made again.
	*
	* @brief Remakes the body
	*/
	void Rigidbody::OnRevision()
	{
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		OnDestroy();
		OnAwake();
	}

	/**
	* [JP]
	* 体を作り直す。
	*
	* @brief 体を作り直す
	*
	* [EN]
	* Remakes the body.
	*
	* @brief Remakes the body
	*/
	void Rigidbody::Dispatch()
	{
		OnRevision();
	}

	/**
	* [JP]
	* 体に力をかける。重さに合わせて少しずつ速くなる。
	*
	* @brief 力をかける
	* @param force かける力。2D はピクセル単位
	* @note  重さで動く体（Dynamic）にしか効かない
	*
	* [EN]
	* Pushes the body with a force; it speeds up gradually, depending on its mass.
	*
	* @brief Adds a force
	* @param force The force; pixels for 2D
	* @note  Only works on a Dynamic body
	*/
	void Rigidbody::AddForce(const Vector3& force)
	{
		/// [JP] 体がない・重さで動く体でないときは使えない
		/// [EN] It cannot be used without a body or on a non-Dynamic body
		if (bodyID_ == ScInvalidBodyID)
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

		/// [JP] 2D はピクセルをメートルに直し、Y を逆にする
		/// [EN] For 2D, turn pixels into meters and flip Y
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetService<Physics>()->AddForce(bodyID_, Vector3(force.x / pixelsPerMeter_, -force.y / pixelsPerMeter_, 0.0f));
			return;
		}

		actor.GetService<Physics>()->AddForce(bodyID_, force);
	}

	/**
	* [JP]
	* 体を一瞬でぽんと押す。
	*
	* @brief 一瞬押す
	* @param impulse 押す強さ。2D はピクセル単位
	* @note  重さで動く体（Dynamic）にしか効かない
	*
	* [EN]
	* Gives the body a quick shove.
	*
	* @brief Adds an impulse
	* @param impulse How hard to shove; pixels for 2D
	* @note  Only works on a Dynamic body
	*/
	void Rigidbody::AddImpulse(const Vector3& impulse)
	{
		/// [JP] 体がない・重さで動く体でないときは使えない
		/// [EN] It cannot be used without a body or on a non-Dynamic body
		if (bodyID_ == ScInvalidBodyID)
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

		/// [JP] 2D はピクセルをメートルに直し、Y を逆にする
		/// [EN] For 2D, turn pixels into meters and flip Y
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetService<Physics>()->AddImpulse(bodyID_, Vector3(impulse.x / pixelsPerMeter_, -impulse.y / pixelsPerMeter_, 0.0f));
			return;
		}

		actor.GetService<Physics>()->AddImpulse(bodyID_, impulse);
	}

	/**
	* [JP]
	* 体を回そうとする力をかける。
	*
	* @brief 回す力をかける
	* @param torque 回す力。2D は Z だけ使う
	* @note  重さで動く体（Dynamic）にしか効かない
	*
	* [EN]
	* Applies a turning force to the body.
	*
	* @brief Adds a torque
	* @param torque The turning force; 2D uses only Z
	* @note  Only works on a Dynamic body
	*/
	void Rigidbody::AddTorque(const Vector3& torque)
	{
		/// [JP] 体がない・重さで動く体でないときは使えない
		/// [EN] It cannot be used without a body or on a non-Dynamic body
		if (bodyID_ == ScInvalidBodyID)
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

		/// [JP] 2D は Z の回転だけを、向きを逆にして使う
		/// [EN] For 2D, use only the Z turn, reversed
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetService<Physics>()->AddTorque(bodyID_, Vector3(0.0f, 0.0f, -torque.z / (pixelsPerMeter_ * pixelsPerMeter_)));
			return;
		}

		actor.GetService<Physics>()->AddTorque(bodyID_, torque);
	}

	/**
	* [JP]
	* 体を一瞬でぽんと回す。
	*
	* @brief 一瞬回す
	* @param angularImpulse 回す強さ。2D は Z だけ使う
	* @note  重さで動く体（Dynamic）にしか効かない
	*
	* [EN]
	* Gives the body a quick spin.
	*
	* @brief Adds a spin
	* @param angularImpulse How hard to spin; 2D uses only Z
	* @note  Only works on a Dynamic body
	*/
	void Rigidbody::AddSpin(const Vector3& angularImpulse)
	{
		/// [JP] 体がない・重さで動く体でないときは使えない
		/// [EN] It cannot be used without a body or on a non-Dynamic body
		if (bodyID_ == ScInvalidBodyID)
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

		/// [JP] 2D は Z の回転だけを、向きを逆にして使う
		/// [EN] For 2D, use only the Z turn, reversed
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetService<Physics>()->AddSpin(bodyID_, Vector3(0.0f, 0.0f, -angularImpulse.z / (pixelsPerMeter_ * pixelsPerMeter_)));
			return;
		}

		actor.GetService<Physics>()->AddSpin(bodyID_, angularImpulse);
	}

	/**
	* [JP]
	* 次の位置と向きまで、この時間で動かす。毎フレーム呼んで使う。
	*
	* @brief 目標まで動かす
	* @param targetPosition 目標の位置
	* @param targetRotation 目標の向き
	* @param elapsedTime    たどり着くまでの時間
	* @note  プログラムで動かす体（Kinematic）にしか効かない
	*
	* [EN]
	* Moves the body to its next position and facing over this time; call it every frame.
	*
	* @brief Moves to a target
	* @param targetPosition The target position
	* @param targetRotation The target facing
	* @param elapsedTime    How long it has to get there
	* @note  Only works on a Kinematic body
	*/
	void Rigidbody::MoveTarget(const Vector3& targetPosition, const Quaternion& targetRotation, Float elapsedTime)
	{
		/// [JP] 体がない・プログラムで動かす体でない・時間が 0 以下なら使えない
		/// [EN] It cannot be used without a body, on a non-Kinematic body, or with no time
		if (bodyID_ == ScInvalidBodyID)
		{
			SC_LOG_ERROR("Rigidbody::MoveTarget を適用できません: ボディが生成されていません。");
			return;
		}

		if (bodyType_ != BodyType::Kinematic)
		{
			SC_LOG_ERROR("Rigidbody::MoveTarget を適用できません: Kinematic のボディにしか効きません。");
			return;
		}

		if (elapsedTime <= 0.0f)
		{
			SC_LOG_ERROR("Rigidbody::MoveTarget を適用できません: elapsedTime は正の値が必要です (現在: {})。", elapsedTime);
			return;
		}

		Actor actor = GetActor();

		/// [JP] 2D は位置と向きを物理の単位に直す
		/// [EN] For 2D, convert the position and facing to physics units
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			Quaternion physicsRotation = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, -2.0f * std::atan2(targetRotation.z, targetRotation.w));
			actor.GetService<Physics>()->MoveTarget(bodyID_, Vector3(targetPosition.x / pixelsPerMeter_, -targetPosition.y / pixelsPerMeter_, 0.0f), physicsRotation, elapsedTime);
			return;
		}

		actor.GetService<Physics>()->MoveTarget(bodyID_, targetPosition, targetRotation, elapsedTime);
	}

	/**
	* [JP]
	* 目標の位置と向きまで、決めた時間をかけて自動で動かす。
	*
	* @brief 時間をかけて動かす
	* @param targetPosition 目標の位置
	* @param targetRotation 目標の向き
	* @param duration       かける時間
	* @note  プログラムで動かす体（Kinematic）にしか効かない
	*
	* [EN]
	* Moves the body to a target position and facing on its own, over a set time.
	*
	* @brief Moves over time
	* @param targetPosition The target position
	* @param targetRotation The target facing
	* @param duration       How long to take
	* @note  Only works on a Kinematic body
	*/
	void Rigidbody::MoveTo(const Vector3& targetPosition, const Quaternion& targetRotation, Float duration)
	{
		/// [JP] 体がない・プログラムで動かす体でない・時間が 0 以下なら使えない
		/// [EN] It cannot be used without a body, on a non-Kinematic body, or with no time
		if (bodyID_ == ScInvalidBodyID)
		{
			SC_LOG_ERROR("Rigidbody::MoveTo を適用できません: ボディが生成されていません。");
			return;
		}

		if (bodyType_ != BodyType::Kinematic)
		{
			SC_LOG_ERROR("Rigidbody::MoveTo を適用できません: Kinematic のボディにしか効きません。");
			return;
		}

		if (duration <= 0.0f)
		{
			SC_LOG_ERROR("Rigidbody::MoveTo を適用できません: duration は正の値が必要です (現在: {})。", duration);
			return;
		}

		Actor actor = GetActor();

		/// [JP] 目標を物理の単位で覚えて、動き始める
		/// [EN] Remember the target in physics units and start moving
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			moveTargetPosition_ = Vector3(targetPosition.x / pixelsPerMeter_, -targetPosition.y / pixelsPerMeter_, 0.0f);
			moveTargetRotation_ = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, -2.0f * Atan2(targetRotation.z, targetRotation.w));
		}
		else
		{
			moveTargetPosition_ = targetPosition;
			moveTargetRotation_ = targetRotation;
		}

		moveTargetRotation_.Normalize();
		moveRemainingTime_ = duration;
		moving_ = true;
	}

	/**
	* [JP]
	* 物理の体の番号を返す。
	*
	* @brief  体の番号を返す
	* @return 体の番号。まだなければ無効な番号
	*
	* [EN]
	* Returns the physics body number.
	*
	* @brief  Returns the body number
	* @return The body number, or the invalid number if there is none yet
	*/
	ScBodyID Rigidbody::BodyID()const
	{
		return bodyID_;
	}

	/**
	* [JP]
	* スクリプトが変えた速さと、自動の移動を体に伝える。
	*
	* @brief 速さと移動を伝える
	* @param elapsedTime 前の物理の更新からの時間
	*
	* [EN]
	* Sends speed changes made by scripts and the automatic move to the body.
	*
	* @brief Applies speed and movement
	* @param elapsedTime Time since the last physics update
	*/
	void Rigidbody::Apply(Float elapsedTime)
	{
		ApplyLinearVelocity();
		ApplyAngularVelocity();

		ApplyMove(elapsedTime);
	}

	/**
	* [JP]
	* 速さの部品が変わっていたら、体の速さに写す。
	*
	* @brief 速さを体に写す
	*
	* [EN]
	* If the velocity part changed, copies it to the body.
	*
	* @brief Applies the velocity
	*/
	void Rigidbody::ApplyLinearVelocity()
	{
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		Actor actor = GetActor();
		LinearVelocity* velocity = actor.GetComponent<LinearVelocity>();
		if (!velocity)
		{
			return;
		}

		/// [JP] スクリプトが変えていなければ何もしない
		/// [EN] Do nothing if no script changed it
		if (velocity->x_ == syncedLinearVelocity_.x && velocity->y_ == syncedLinearVelocity_.y && velocity->z_ == syncedLinearVelocity_.z)
		{
			return;
		}

		/// [JP] 変えた速さに足場の分を足して、体に伝える。2D はメートルに直す
		/// [EN] Add the foothold part to the new speed and send it to the body; meters for 2D
		syncedLinearVelocity_ = Vector3(velocity->x_, velocity->y_, velocity->z_);

		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetService<Physics>()->BodyLinearVelocity(bodyID_, Vector3(syncedLinearVelocity_.x / pixelsPerMeter_, -syncedLinearVelocity_.y / pixelsPerMeter_, 0.0f) + inheritedLinearVelocity_);
			return;
		}

		actor.GetService<Physics>()->BodyLinearVelocity(bodyID_, syncedLinearVelocity_ + inheritedLinearVelocity_);
	}

	/**
	* [JP]
	* 回る速さの部品が変わっていたら、体の回る速さに写す。
	*
	* @brief 回る速さを体に写す
	*
	* [EN]
	* If the spin part changed, copies it to the body.
	*
	* @brief Applies the spin
	*/
	void Rigidbody::ApplyAngularVelocity()
	{
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		Actor actor = GetActor();
		AngularVelocity* velocity = actor.GetComponent<AngularVelocity>();
		if (!velocity)
		{
			return;
		}

		/// [JP] スクリプトが変えていなければ何もしない
		/// [EN] Do nothing if no script changed it
		if (velocity->x_ == syncedAngularVelocity_.x && velocity->y_ == syncedAngularVelocity_.y && velocity->z_ == syncedAngularVelocity_.z)
		{
			return;
		}

		/// [JP] 変えた回る速さに足場の分を足して、体に伝える
		/// [EN] Add the foothold part to the new spin and send it to the body
		syncedAngularVelocity_ = Vector3(velocity->x_, velocity->y_, velocity->z_);

		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetService<Physics>()->BodyAngularVelocity(bodyID_, Vector3(0.0f, 0.0f, -syncedAngularVelocity_.z) + inheritedAngularVelocity_);
			return;
		}

		actor.GetService<Physics>()->BodyAngularVelocity(bodyID_, syncedAngularVelocity_ + inheritedAngularVelocity_);
	}

	/**
	* [JP]
	* MoveTo で決めた目標に向けて、少しずつ動かす。
	*
	* @brief 目標へ少し動かす
	* @param elapsedTime 前の物理の更新からの時間
	*
	* [EN]
	* Moves a little toward the target set by MoveTo.
	*
	* @brief Steps toward the target
	* @param elapsedTime Time since the last physics update
	*/
	void Rigidbody::ApplyMove(Float elapsedTime)
	{
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		/// [JP] 動いていない、またはプログラムで動かす体でなければ終わる
		/// [EN] Stop if it is not moving or not Kinematic
		if (!moving_)
		{
			return;
		}

		if (bodyType_ != BodyType::Kinematic)
		{
			moving_ = false;
			return;
		}

		Physics& physics = *GetActor().GetService<Physics>();

		/// [JP] 時間が来たら止めて終わる
		/// [EN] Once time is up, stop it and finish
		if (moveRemainingTime_ <= 0.0f)
		{
			physics.BodyLinearVelocity(bodyID_, Vector3::Zero);
			physics.BodyAngularVelocity(bodyID_, Vector3::Zero);
			moving_ = false;
			return;
		}

		/// [JP] 残り時間に合わせて、目標までの一部だけ進める
		/// [EN] Move part of the way to the target, based on the time left
		Matrix bodyMatrix = physics.BodyTransform(bodyID_);
		Vector3 currentPosition = bodyMatrix.Translation();
		Quaternion currentRotation = Quaternion::CreateFromRotationMatrix(bodyMatrix);

		Float ratio = std::min(elapsedTime / moveRemainingTime_, 1.0f);
		Vector3 stepPosition = Vector3::Lerp(currentPosition, moveTargetPosition_, ratio);
		Quaternion stepRotation = Quaternion::Slerp(currentRotation, moveTargetRotation_, ratio);

		physics.MoveTarget(bodyID_, stepPosition, stepRotation, elapsedTime);
		moveRemainingTime_ -= elapsedTime;
	}

	/**
	* [JP]
	* 動く足場に乗っていたら、足場の動きを体に足す。
	*
	* @brief 足場の動きを足す
	* @param elapsedTime 前の物理の更新からの時間
	*
	* [EN]
	* If it stands on a moving foothold, adds the foothold's motion to the body.
	*
	* @brief Adds the foothold motion
	* @param elapsedTime Time since the last physics update
	*/
	void Rigidbody::ApplyFoothold(Float elapsedTime)
	{
		if (bodyID_ == ScInvalidBodyID || !followFoothold_ || bodyType_ != BodyType::Dynamic || isTrigger_ || elapsedTime <= 0.0f)
		{
			return;
		}

		Actor actor = GetActor();
		Physics& physics = *actor.GetService<Physics>();

		/// [JP] 真下に足場があるかを探す
		/// [EN] Look for a foothold right underneath
		Vector3 nextLinearVelocity = { 0.0f, 0.0f, 0.0f };
		Vector3 nextAngularVelocity = { 0.0f, 0.0f, 0.0f };

		ScBodyID footholdID;
		physics.BodyFoothold(bodyID_, ToRadians(footholdSlopeAngle_), 0.05f, footholdID);

		/// [JP] 足場があれば、足場がこの時間に動く分を速さにする
		/// [EN] With a foothold, turn how far it moves in this time into speed
		if (footholdID != ScInvalidBodyID)
		{
			Vector3 displacement;
			physics.BodyPointDisplacement(footholdID, physics.BodyTransform(bodyID_).Translation(), elapsedTime, displacement);
			nextLinearVelocity = displacement / elapsedTime;

			Vector3 footholdAngularVelocity = physics.BodyAngularVelocity(footholdID);

			/// [JP] 回転は、2D なら Z、3D なら Y だけ受け取る
			/// [EN] Take only the Z turn for 2D, or the Y turn for 3D
			if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
			{
				nextAngularVelocity = Vector3(0.0f, 0.0f, footholdAngularVelocity.z);
			}
			else
			{
				nextAngularVelocity = Vector3(0.0f, footholdAngularVelocity.y, 0.0f);
			}
		}
		else
		{
			/// [JP] 足場がなければ、横の速さだけを持ち越す
			/// [EN] Without a foothold, carry over only the sideways speed
			nextLinearVelocity = Vector3(inheritedLinearVelocity_.x, 0.0f, inheritedLinearVelocity_.z);
			nextAngularVelocity = inheritedAngularVelocity_;
		}

		/// [JP] 前の足場の分を引いて、新しい足場の分を足す
		/// [EN] Take off the old foothold part and add the new one
		physics.BodyLinearVelocity(bodyID_, physics.BodyLinearVelocity(bodyID_) - inheritedLinearVelocity_ + nextLinearVelocity);
		physics.BodyAngularVelocity(bodyID_, physics.BodyAngularVelocity(bodyID_) - inheritedAngularVelocity_ + nextAngularVelocity);

		inheritedLinearVelocity_ = nextLinearVelocity;
		inheritedAngularVelocity_ = nextAngularVelocity;
	}
}
