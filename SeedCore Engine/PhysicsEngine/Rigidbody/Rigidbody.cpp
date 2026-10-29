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
	* [EN]
	* Adds LinearVelocity and AngularVelocity to the actor the moment
	* this component is added, so the velocities the body is kept in step
	* with exist before any script's OnAwake or OnStart reads them. A
	* velocity the actor already has is kept as it is.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このコンポーネントが追加された瞬間に、actor へ LinearVelocity と
	* AngularVelocity を追加する。ボディと同期させる速度が、どの
	* スクリプトの OnAwake・OnStart が読むより前に存在するようにする
	* ため。actor が既に持っている速度はそのまま残す。
	*/
	void Rigidbody::OnAttach()
	{
		/// [EN] AddComponent returns the existing instance when the actor already has one, so a velocity set before this component was added is not reset.
		/// [JP] actor が既に持っていれば AddComponent は既存のものを返すので、このコンポーネントより先に設定された速度は初期化されない。
		Actor actor = GetActor();
		actor.AddComponent<LinearVelocity>();
		actor.AddComponent<AngularVelocity>();
	}

	/**
	* [EN]
	* Removes LinearVelocity and AngularVelocity from the actor the moment
	* this component is removed, undoing OnAttach.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このコンポーネントが外される瞬間に、actor から LinearVelocity と
	* AngularVelocity を外し、OnAttach を元に戻す。
	*/
	void Rigidbody::OnDetach()
	{
		Actor actor = GetActor();
		actor.RemoveComponent<LinearVelocity>();
		actor.RemoveComponent<AngularVelocity>();
	}

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

			/// [EN] Without a collider the body still needs a shape, so a small sphere stands in.
			/// [JP] コライダーが無くてもボディには形状が要るので、小さな球で代用する。
			/// [EN] Like SphereCollider, the radius follows the largest axis of the actor's own Scale combined with its parent's scale.
			/// [JP] SphereCollider と同じく、半径は Actor 自身の Scale と親のスケールを掛けたものの最も大きい軸に合わせる。
			const Scale* scale = actor.GetComponent<Scale>();
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
			return actor.GetService<Physics>()->CreateSphereShape(defaultShapeRadius_ * Max(Abs(worldScale.x), Abs(worldScale.y), Abs(worldScale.z)));
		}();

		/// [EN] Starts with every axis free and removes each frozen one.
		/// [JP] 全ての軸を自由にした状態から、固定した軸を1つずつ外す。
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

		/// [EN] Shape, pose and EntityID, then the motion type and the layer packed from it and the actor's layer.
		/// [JP] 形状・姿勢・EntityID、続いて運動タイプと、それと Actor のレイヤーからパックしたレイヤー。
		RigidbodyDesc desc;
		desc.shape_ = shapeHandle_;
		PhysicsSystem::ApplyTransform(actor, desc);
		desc.motionType_ = ToMotionType(bodyType_);
		desc.allowMotionTypeChange_ = true;
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

		/// [EN] A trigger is marked in the layer as well, so the layer filters can let it meet static and kinematic bodies.
		/// [JP] トリガーはレイヤーにも印を付け、レイヤーフィルターがスタティック・キネマティックのボディと組ませられるようにする。
		if (isTrigger_)
		{
			desc.layer_ |= Layers::SENSOR;
		}

		/// [EN] A canvas body lives on the z = 0 plane and only collides with other canvas bodies.
		/// [JP] Canvas のボディは z = 0 の平面上にあり、他の Canvas のボディとだけ衝突する。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			/// [EN] Only X/Y movement and rotation about Z remain; the 3D freeze settings are remapped below.
			/// [JP] 残るのは X/Y の移動と Z 軸まわりの回転だけ。3D 用の固定設定は下で置き換える。
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

			/// [EN] The canvas angle is the rotation about Z, so the Z freeze locks it.
			/// [JP] Canvas の角度は Z 軸まわりの回転なので、Z の固定でそれを固定する。
			if (freezeRotationZ_)
			{
				desc.allowedDOFs_ &= ~ScAllowedDOFsRotationZ;
			}
		}

		bodyID_ = actor.GetService<Physics>()->CreateRigidbody(desc);

		/// [EN] A LinearVelocity and AngularVelocity set before play become the body's initial velocity and angular velocity.
		/// [JP] プレイ前に設定した LinearVelocity と AngularVelocity は、ボディの初速と初期の角速度になる。
		ApplyLinearVelocity();
		ApplyAngularVelocity();
	}

	/**
	* [EN]
	* Copies the simulated pose of the body back to the actor's
	* Position and Rotation, its linear velocity back to LinearVelocity,
	* and its angular velocity back to AngularVelocity. Both velocities
	* leave out what the foothold gives. The step's damping slows the
	* whole velocity, the foothold's part included, so that part is
	* first scaled by the same factor to match what the body still
	* carries.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シミュレーション後のボディの姿勢を Actor の Position と Rotation へ、
	* 速度を LinearVelocity へ、角速度を AngularVelocity へ書き戻す。
	* どちらの速度も、足場が与えている分は除く。ステップの減衰は足場の分も
	* 含めた速度全体を弱めるので、先に足場の分へ同じ割合を掛け、ボディが
	* 実際にまだ持っている分と揃える。
	*/
	void Rigidbody::OnFixedTick(Float elapsedTime)
	{
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		/// [EN] Jolt slows a Dynamic body's whole velocity by max(0, 1 - drag * elapsedTime) each step, so the part the foothold gave shrinks by that same factor.
		/// [JP] Jolt は Dynamic のボディの速度全体を毎ステップ max(0, 1 - 抵抗 * elapsedTime) 倍に弱めるので、足場から受けた分も同じ割合で小さくなる。
		/// [EN] Only a body ApplyFoothold carries holds a foothold's part, under the same conditions.
		/// [JP] 足場の分を持つのは ApplyFoothold が運ぶボディだけなので、条件もそれと揃える。
		if (followFoothold_ && bodyType_ == BodyType::Dynamic && !isTrigger_)
		{
			inheritedLinearVelocity_ *= Max(0.0f, 1.0f - linearDrag_ * elapsedTime);
			inheritedAngularVelocity_ *= Max(0.0f, 1.0f - angularDrag_ * elapsedTime);
		}

		Actor actor = GetActor();

		/// [EN] The velocity after the step is copied back and remembered, so ApplyLinearVelocity can tell a LinearVelocity written by code from this one.
		/// [JP] ステップ後の速度を書き戻して覚えておき、ApplyLinearVelocity がコードから書かれた LinearVelocity とこれを見分けられるようにする。
		if (LinearVelocity* velocity = actor.GetComponent<LinearVelocity>())
		{
			/// [EN] What the foothold gives is left out, so LinearVelocity is the body's velocity relative to its foothold.
			/// [JP] 足場が与えている分を除き、LinearVelocity を足場に対するボディの速度にする。
			Vector3 bodyVelocity = actor.GetService<Physics>()->BodyLinearVelocity(bodyID_) - inheritedLinearVelocity_;

			/// [EN] Canvas bodies go back from meters per second (Y up) to pixels per second (Y down).
			/// [JP] Canvas のボディは、メートル毎秒(Y 上向き)からピクセル毎秒(Y 下向き)へ戻す。
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

		/// [EN] The angular velocity after the step is copied back and remembered the same way, for ApplyAngularVelocity.
		/// [JP] ステップ後の角速度も同じように書き戻して覚えておき、ApplyAngularVelocity が見分けられるようにする。
		if (AngularVelocity* velocity = actor.GetComponent<AngularVelocity>())
		{
			/// [EN] What the foothold gives is left out, so AngularVelocity is the body's angular velocity relative to its foothold.
			/// [JP] 足場が与えている分を除き、AngularVelocity を足場に対するボディの角速度にする。
			Vector3 bodyAngularVelocity = actor.GetService<Physics>()->BodyAngularVelocity(bodyID_) - inheritedAngularVelocity_;

			/// [EN] A canvas body only turns about Z; the flipped Y reverses the direction of the turn, and an angle needs no pixel conversion.
			/// [JP] Canvas のボディは Z 軸まわりにしか回らない。Y の反転で回る向きが逆になり、角度にはピクセルへの換算は要らない。
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

		/// [EN] With neither Position nor Rotation there is nothing to write back.
		/// [JP] Position も Rotation も無ければ、書き戻す先が無い。
		Position* position = actor.GetComponent<Position>();
		Rotation* rotation = actor.GetComponent<Rotation>();
		if (!position && !rotation)
		{
			return;
		}

		Matrix bodyMatrix = actor.GetService<Physics>()->BodyTransform(bodyID_);
		Vector3 outPosition = bodyMatrix.Translation();
		Quaternion outRotation = Quaternion::CreateFromRotationMatrix(bodyMatrix);

		/// [EN] The body pose is in world space, while Position and Rotation are relative to the parent, so it is carried back by the inverse of the parent's world matrix; a root actor's parent space is the world itself.
		/// [JP] ボディの姿勢はワールド空間にあり、Position と Rotation は親から見た値なので、親のワールド行列の逆で戻す。ルートの Actor にとって、親の空間はワールドそのもの。
		Actor parentActor = actor.Parent();
		Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
		Vector3 parentScale;
		Quaternion parentRotation;
		Vector3 parentTranslation;
		parentMatrix.Decompose(parentScale, parentRotation, parentTranslation);

		Matrix inverseParentMatrix = parentMatrix.Invert();
		Quaternion inverseParentRotation;
		parentRotation.Inverse(inverseParentRotation);

		/// [EN] Canvas bodies go back from meters (Y up) to pixels (Y down).
		/// [JP] Canvas のボディは、メートル(Y 上向き)からピクセル(Y 下向き)へ戻す。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			/// [EN] The body only turns about Z, so the angle is 2·atan2(z, w), negated with the flipped Y and stored as a rotation about Z.
			/// [JP] ボディは Z 軸まわりにしか回らないので、角度は 2·atan2(z, w)。Y の反転に合わせて符号を反転し、Z 軸まわりの回転として格納する。
			Quaternion canvasRotation = Quaternion::CreateFromAxisAngle(Vector3::UnitZ, -2.0f * std::atan2(outRotation.z, outRotation.w));

			if (position)
			{
				Vector3 localPosition = Vector3::Transform(Vector3(outPosition.x * pixelsPerMeter_, -outPosition.y * pixelsPerMeter_, 0.0f), inverseParentMatrix);
				position->x_ = localPosition.x;
				position->y_ = localPosition.y;
			}

			if (rotation)
			{
				Transform::Quat(*rotation, canvasRotation * inverseParentRotation);
			}

			return;
		}

		if (position)
		{
			Transform::Vector(*position, Vector3::Transform(outPosition, inverseParentMatrix));
		}

		/// [EN] Rotation is stored as a normalized quaternion.
		/// [JP] Rotation は正規化済みクォータニオンで持つ。
		if (rotation)
		{
			Transform::Quat(*rotation, outRotation * inverseParentRotation);
		}
	}

	/**
	* [EN]
	* Destroys the body, releases the reference to its shape, and forgets
	* what the foothold gave.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディを破棄し、形状への参照を解放し、足場から受けていた分を忘れる。
	*/
	void Rigidbody::OnDestroy()
	{
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		/// [EN] Only this component's reference is released; the collider keeps its own.
		/// [JP] 解放するのはこのコンポーネントの参照だけで、コライダーは自分の参照を持ち続ける。
		Actor actor = GetActor();
		actor.GetService<Physics>()->DestroyBody(bodyID_);
		actor.GetService<Physics>()->ReleaseShape(shapeHandle_);

		bodyID_ = ScInvalidBodyID;

		/// [EN] A new body starts on no foothold, so it is given nothing yet.
		/// [JP] 新しいボディは足場に乗っていない状態から始めるので、まだ何も受けていない。
		inheritedLinearVelocity_ = Vector3::Zero;
		inheritedAngularVelocity_ = Vector3::Zero;
	}

	/**
	* [EN]
	* Rebuilds the body from the current settings, so a changed field
	* takes effect. Does nothing while no body exists.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の設定でボディを作り直し、変更したフィールドを効かせる。
	* ボディが無い間は何もしない。
	*/
	void Rigidbody::OnRevision()
	{
		/// [EN] Before OnAwake or after OnDestroy there is no body to rebuild, and OnAwake builds it with the current settings anyway.
		/// [JP] OnAwake 前や OnDestroy 後は作り直すボディが無く、OnAwake がいずれ現在の設定で作る。
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		OnDestroy();
		OnAwake();
	}

	/**
	* [EN]
	* Applies fields changed from code. An edit in the inspector runs
	* OnRevision on its own, but a field written by a script takes
	* effect only once this is called.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コードから変えたフィールドを反映する。インスペクタでの編集は
	* 自動で OnRevision が走るが、スクリプトから書き換えたフィールドは
	* これを呼んだときに初めて効く。
	*/
	void Rigidbody::Dispatch()
	{
		OnRevision();
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

		/// [EN] Canvas bodies go from pixels (Y down) to meters (Y up); the body stays on the z = 0 plane.
		/// [JP] Canvas のボディは、ピクセル(Y 下向き)からメートル(Y 上向き)へ直す。ボディは z = 0 の平面上に留まる。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetService<Physics>()->AddForce(bodyID_, Vector3(force.x / pixelsPerMeter_, -force.y / pixelsPerMeter_, 0.0f));
			return;
		}

		actor.GetService<Physics>()->AddForce(bodyID_, force);
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

		/// [EN] Canvas bodies go from pixels (Y down) to meters (Y up); the body stays on the z = 0 plane.
		/// [JP] Canvas のボディは、ピクセル(Y 下向き)からメートル(Y 上向き)へ直す。ボディは z = 0 の平面上に留まる。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetService<Physics>()->AddImpulse(bodyID_, Vector3(impulse.x / pixelsPerMeter_, -impulse.y / pixelsPerMeter_, 0.0f));
			return;
		}

		actor.GetService<Physics>()->AddImpulse(bodyID_, impulse);
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

		/// [EN] A canvas body only turns about Z, and the canvas angle runs opposite to the physics angle because of the flipped Y, so Z is negated.
		/// [JP] Canvas のボディは Z 軸まわりにしか回らず、Y の反転により Canvas の角度は物理の角度と逆向きなので、Z を反転する。
		/// [EN] Torque carries length squared (kg·m²/s²), so pixels come back to meters by dividing twice.
		/// [JP] トルクは長さの2乗(kg·m²/s²)を含むので、ピクセルからメートルへは2回割って戻す。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetService<Physics>()->AddTorque(bodyID_, Vector3(0.0f, 0.0f, -torque.z / (pixelsPerMeter_ * pixelsPerMeter_)));
			return;
		}

		actor.GetService<Physics>()->AddTorque(bodyID_, torque);
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

		/// [EN] A canvas body only turns about Z, and the canvas angle runs opposite to the physics angle because of the flipped Y, so Z is negated.
		/// [JP] Canvas のボディは Z 軸まわりにしか回らず、Y の反転により Canvas の角度は物理の角度と逆向きなので、Z を反転する。
		/// [EN] Angular impulse carries length squared (kg·m²/s), so pixels come back to meters by dividing twice.
		/// [JP] 角力積は長さの2乗(kg·m²/s)を含むので、ピクセルからメートルへは2回割って戻す。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetService<Physics>()->AddSpin(bodyID_, Vector3(0.0f, 0.0f, -angularImpulse.z / (pixelsPerMeter_ * pixelsPerMeter_)));
			return;
		}

		actor.GetService<Physics>()->AddSpin(bodyID_, angularImpulse);
	}

	/**
	* [EN]
	* Moves the body so that it reaches the target position and rotation
	* after elapsedTime, carrying along whatever stands on it. Call it
	* every fixed step with that step's elapsedTime. On a canvas body the
	* position is given in pixels with Y down and its Z is ignored, and
	* only the rotation about Z is used. Only a Kinematic body is affected.
	* The body moves and turns by the velocity and angular velocity this
	* sets, which it keeps until the next call or until LinearVelocity or
	* AngularVelocity is written, so the body does not stop at the target
	* by itself.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* elapsedTime 後に目標の位置と回転へ着くようボディを動かし、上に乗って
	* いるものも一緒に運ぶ。固定ステップごとに、そのステップの elapsedTime
	* を渡して呼ぶ。Canvas のボディでは位置をピクセル単位・Y 下向きで与えて
	* Z は無視し、回転は Z 軸まわりだけを使う。効くのは Kinematic のボディ
	* だけ。ボディはこれが設定した速度と角速度で動き、それらは次の呼び出しか
	* LinearVelocity・AngularVelocity への書き込みまで保たれるので、目標で
	* 自然には止まらない。
	*/
	void Rigidbody::MoveTarget(const Vector3& targetPosition, const Quaternion& targetRotation, Float elapsedTime)
	{
		/// [EN] The body exists only between OnAwake and OnDestroy, and only a Kinematic body follows a target.
		/// [JP] ボディが存在するのは OnAwake から OnDestroy までの間だけで、目標に従うのは Kinematic のボディだけ。
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
			actor.GetService<Physics>()->MoveTarget(bodyID_, Vector3(targetPosition.x / pixelsPerMeter_, -targetPosition.y / pixelsPerMeter_, 0.0f), physicsRotation, elapsedTime);
			return;
		}

		actor.GetService<Physics>()->MoveTarget(bodyID_, targetPosition, targetRotation, elapsedTime);
	}

	/**
	* [EN]
	* Moves the body to the target position and rotation over duration
	* seconds and stops it there, carrying along whatever stands on it.
	* Call it once; each fixed step then advances the body by its share
	* of the remaining distance, and the step after arrival sets the
	* velocity and angular velocity back to zero. Calling it again while
	* moving replaces the target. On a canvas body the position is given
	* in pixels with Y down and its Z is ignored, and only the rotation
	* about Z is used. Only a Kinematic body is affected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* duration 秒かけて目標の位置と回転までボディを動かして止め、上に乗って
	* いるものも一緒に運ぶ。呼ぶのは一度だけでよく、以降は固定ステップごとに
	* 残りの距離のうちそのステップの分だけ進み、着いた次のステップで速度と
	* 角速度を 0 に戻す。移動中にもう一度呼ぶと目標を置き換える。Canvas の
	* ボディでは位置をピクセル単位・Y 下向きで与えて Z は無視し、回転は Z 軸
	* まわりだけを使う。効くのは Kinematic のボディだけ。
	*/
	void Rigidbody::MoveTo(const Vector3& targetPosition, const Quaternion& targetRotation, Float duration)
	{
		/// [EN] The body exists only between OnAwake and OnDestroy, and only a Kinematic body follows a target.
		/// [JP] ボディが存在するのは OnAwake から OnDestroy までの間だけで、目標に従うのは Kinematic のボディだけ。
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

		/// [EN] Each step's velocity is the remaining distance divided by the remaining time, so the move must have a positive length.
		/// [JP] 各ステップの速度は残りの距離を残り時間で割ったものなので、移動時間は正でなければならない。
		if (duration <= 0.0f)
		{
			SC_LOG_ERROR("Rigidbody::MoveTo を適用できません: duration は正の値が必要です (現在: {})。", duration);
			return;
		}

		Actor actor = GetActor();

		/// [EN] The target is kept in physics space, so every step works on the body's own pose without converting again.
		/// [JP] 目標は物理空間で持っておき、各ステップは変換し直さずにボディ自身の姿勢と比べる。
		/// [EN] A canvas body goes from pixels (Y down) to meters (Y up) on the z = 0 plane, and its angle 2·atan2(z, w) is negated with the flipped Y.
		/// [JP] Canvas のボディは、ピクセル(Y 下向き)から z = 0 平面上のメートル(Y 上向き)へ変換し、角度 2·atan2(z, w) は Y の反転に合わせて符号を反転する。
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
	* [EN]
	* Returns the Jolt ID of the body; invalid before OnAwake and after
	* OnDestroy.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディの Jolt ID を返す。OnAwake 前と OnDestroy 後は無効な ID。
	*/
	ScBodyID Rigidbody::BodyID()const
	{
		return bodyID_;
	}

	/**
	* [EN]
	* Hands what code has changed since the last step (bodyType_,
	* isTrigger_, and the actor's LinearVelocity and AngularVelocity) to
	* the body, so it takes effect in the next step, and advances a move
	* started by MoveTo by one step of elapsedTime. Called by
	* PhysicsSystem before every fixed step.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Actor の LinearVelocity と AngularVelocity をボディへ渡し、MoveTo で
	* 始めた移動を elapsedTime の1ステップ分進める。
	* 固定ステップの前に PhysicsSystem から毎回呼ばれる。
	*/
	void Rigidbody::Apply(Float elapsedTime)
	{
		ApplyLinearVelocity();
		ApplyAngularVelocity();

		/// [EN] The move comes last, so while it is in progress its velocity wins over a LinearVelocity or AngularVelocity written by code.
		/// [JP] 移動は最後に行い、移動中はコードから書かれた LinearVelocity・AngularVelocity よりも移動の速度を優先する。
		ApplyMove(elapsedTime);
	}

	/**
	* [EN]
	* Hands the actor's LinearVelocity to the body when it differs from
	* the value last copied back. On a canvas body the velocity is in
	* pixels per second with Y down, and its Z is ignored. The value is
	* relative to the foothold, so the body receives it with what the
	* foothold gives added. A Static body is not affected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Actor の LinearVelocity が前回書き戻した値と違えばボディへ渡す。
	* Canvas のボディでは速度をピクセル毎秒・Y 下向きで与え、Z は無視する。
	* 値は足場に対するものなので、足場が与えている分を足してボディへ渡す。
	* Static のボディには効かない。
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

		/// [EN] A LinearVelocity equal to the last exchanged value was not touched by code, and the body keeps whatever velocity it has (including one from MoveTarget).
		/// [JP] 最後にやり取りした値と同じ LinearVelocity はコードが触っていないので、ボディは今の速度(MoveTarget によるものも含む)を保つ。
		if (velocity->x_ == syncedLinearVelocity_.x && velocity->y_ == syncedLinearVelocity_.y && velocity->z_ == syncedLinearVelocity_.z)
		{
			return;
		}

		syncedLinearVelocity_ = Vector3(velocity->x_, velocity->y_, velocity->z_);

		/// [EN] The written velocity is relative to the foothold, so the body receives it with what the foothold gives added back.
		/// [JP] 書かれた速度は足場に対する値なので、足場が与えている分を足し戻してボディへ渡す。
		/// [EN] A canvas velocity goes from pixels per second (Y down) to meters per second (Y up) on the z = 0 plane.
		/// [JP] Canvas の速度は、ピクセル毎秒(Y 下向き)から z = 0 平面上のメートル毎秒(Y 上向き)へ変換する。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetService<Physics>()->BodyLinearVelocity(bodyID_, Vector3(syncedLinearVelocity_.x / pixelsPerMeter_, -syncedLinearVelocity_.y / pixelsPerMeter_, 0.0f) + inheritedLinearVelocity_);
			return;
		}

		actor.GetService<Physics>()->BodyLinearVelocity(bodyID_, syncedLinearVelocity_ + inheritedLinearVelocity_);
	}

	/**
	* [EN]
	* Hands the actor's AngularVelocity to the body when it differs from
	* the value last copied back. On a canvas body only Z is used, as the
	* in-plane angular velocity in the same direction as the canvas
	* angle. The value is relative to the foothold, so the body receives
	* it with what the foothold gives added. A Static body is not
	* affected.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Actor の AngularVelocity が前回書き戻した値と違えばボディへ渡す。
	* Canvas のボディでは Z だけを、Canvas の角度と同じ向きの平面内の角速度
	* として使う。値は足場に対するものなので、足場が与えている分を足して
	* ボディへ渡す。Static のボディには効かない。
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

		/// [EN] An AngularVelocity equal to the last exchanged value was not touched by code, and the body keeps whatever angular velocity it has (including one from MoveTarget).
		/// [JP] 最後にやり取りした値と同じ AngularVelocity はコードが触っていないので、ボディは今の角速度(MoveTarget によるものも含む)を保つ。
		if (velocity->x_ == syncedAngularVelocity_.x && velocity->y_ == syncedAngularVelocity_.y && velocity->z_ == syncedAngularVelocity_.z)
		{
			return;
		}

		syncedAngularVelocity_ = Vector3(velocity->x_, velocity->y_, velocity->z_);

		/// [EN] The written angular velocity is relative to the foothold, so the body receives it with what the foothold gives added back.
		/// [JP] 書かれた角速度は足場に対する値なので、足場が与えている分を足し戻してボディへ渡す。
		/// [EN] A canvas body only turns about Z; the flipped Y reverses the direction of the turn, and an angle needs no pixel conversion.
		/// [JP] Canvas のボディは Z 軸まわりにしか回らない。Y の反転で回る向きが逆になり、角度にはピクセルへの換算は要らない。
		if (actor.GetComponent<RectCollider>() || actor.GetComponent<CircleCollider>())
		{
			actor.GetService<Physics>()->BodyAngularVelocity(bodyID_, Vector3(0.0f, 0.0f, -syncedAngularVelocity_.z) + inheritedAngularVelocity_);
			return;
		}

		actor.GetService<Physics>()->BodyAngularVelocity(bodyID_, syncedAngularVelocity_ + inheritedAngularVelocity_);
	}

	/**
	* [EN]
	* Advances a move started by MoveTo by one step of elapsedTime:
	* targets the point that splits the remaining distance by this
	* step's share of the remaining time, and once the time is used up,
	* sets the velocity and angular velocity back to zero so the body
	* stays at the target.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* MoveTo で始めた移動を elapsedTime の1ステップ分進める。残りの距離を、
	* 残り時間に占めるこのステップの割合で分けた点を目標にし、時間を使い
	* 切ったら速度と角速度を 0 に戻してボディを目標に留める。
	*/
	void Rigidbody::ApplyMove(Float elapsedTime)
	{
		if (bodyID_ == ScInvalidBodyID)
		{
			return;
		}

		if (!moving_)
		{
			return;
		}

		/// [EN] A body switched away from Kinematic during the move no longer follows a target, so the move ends there.
		/// [JP] 移動中に Kinematic 以外へ切り替えられたボディは目標に従わないので、移動はそこで終わる。
		if (bodyType_ != BodyType::Kinematic)
		{
			moving_ = false;
			return;
		}

		Physics& physics = *GetActor().GetService<Physics>();

		/// [EN] The body reached the target in the last step; a kinematic body keeps its velocity until it is overwritten, so it is set back to zero here.
		/// [JP] ボディは直前のステップで目標に着いている。Kinematic のボディは上書きされるまで速度を保つので、ここで 0 に戻す。
		if (moveRemainingTime_ <= 0.0f)
		{
			physics.BodyLinearVelocity(bodyID_, Vector3::Zero);
			physics.BodyAngularVelocity(bodyID_, Vector3::Zero);
			moving_ = false;
			return;
		}

		Matrix bodyMatrix = physics.BodyTransform(bodyID_);
		Vector3 currentPosition = bodyMatrix.Translation();
		Quaternion currentRotation = Quaternion::CreateFromRotationMatrix(bodyMatrix);

		/// [EN] This step covers elapsedTime out of the remaining time, so it goes that share of the way from the current pose; the last step's share is 1 and lands exactly on the target.
		/// [JP] このステップは残り時間のうち elapsedTime の分を受け持つので、今の姿勢からその割合だけ進む。最後のステップは割合が 1 になり、ちょうど目標に着く。
		Float ratio = std::min(elapsedTime / moveRemainingTime_, 1.0f);
		Vector3 stepPosition = Vector3::Lerp(currentPosition, moveTargetPosition_, ratio);
		Quaternion stepRotation = Quaternion::Slerp(currentRotation, moveTargetRotation_, ratio);

		physics.MoveTarget(bodyID_, stepPosition, stepRotation, elapsedTime);
		moveRemainingTime_ -= elapsedTime;
	}

	/**
	* [EN]
	* Carries the body along with the body it stands on for the coming
	* step of elapsedTime. The velocity the foothold gave in the last
	* step is swapped for the one it gives in this step, so only a
	* change in the foothold's motion reaches the body and nothing
	* builds up. In the air the horizontal velocity and the turn carried
	* so far are kept, and the step's damping slows them along with the
	* body's own, while the vertical velocity stays in the body's own.
	* Called by PhysicsSystem after every Rigidbody has run Apply, so each
	* foothold's velocity for the step is already set.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* これから進む elapsedTime の1ステップの間、乗っているボディと一緒に
	* このボディを運ぶ。前のステップで足場から受けた速度を、このステップで
	* 受ける速度に入れ替えるので、ボディに届くのは足場の動きの変化分だけで、
	* 何も溜まっていかない。空中では、それまで受けていた水平の速度と回転を
	* 残し、ステップの減衰がボディ自身の速度と一緒にそれを弱める。垂直の
	* 速度はボディ自身の速度に残す。
	* 全 Rigidbody の Apply が済み、各足場のこのステップの速度が決まった後に
	* PhysicsSystem から呼ばれる。
	*/
	void Rigidbody::ApplyFoothold(Float elapsedTime)
	{
		/// [EN] Only a solid Dynamic body that asks for it is carried, and the step must have a length to turn a displacement into a velocity.
		/// [JP] 運ぶのは、それを求めている実体のある Dynamic のボディだけ。移動量を速度に直すため、ステップには長さが要る。
		if (bodyID_ == ScInvalidBodyID || !followFoothold_ || bodyType_ != BodyType::Dynamic || isTrigger_ || elapsedTime <= 0.0f)
		{
			return;
		}

		Actor actor = GetActor();
		Physics& physics = *actor.GetService<Physics>();

		Vector3 nextLinearVelocity = { 0.0f, 0.0f, 0.0f };
		Vector3 nextAngularVelocity = { 0.0f, 0.0f, 0.0f };

		/// [EN] A short sweep is enough, since a body resting on its foothold is already touching it.
		/// [JP] 足場の上で止まっているボディは既に触れているので、短い掃引で足りる。
		ScBodyID footholdID;
		physics.BodyFoothold(bodyID_, ToRadians(footholdSlopeAngle_), 0.05f, footholdID);

		if (footholdID != ScInvalidBodyID)
		{
			/// [EN] The point of the foothold under the body's origin moves this far over the step; as a velocity, that is what carries the body exactly along, around a turn as well.
			/// [JP] ボディの原点にある足場の点が、このステップでこれだけ動く。それを速度にしたものが、回転も含めてボディをちょうど運ぶ速度になる。
			Vector3 displacement;
			physics.BodyPointDisplacement(footholdID, physics.BodyTransform(bodyID_).Translation(), elapsedTime, displacement);
			nextLinearVelocity = displacement / elapsedTime;

			/// [EN] Only the turn about the up axis is passed on, so a tilting foothold does not tip the body over; a canvas body turns only about Z, so that axis is passed on instead.
			/// [JP] 渡すのは上向きの軸まわりの回転だけで、足場が傾いてもボディは倒さない。Canvas のボディは Z 軸まわりにしか回らないので、代わりにその軸の回転を渡す。
			Vector3 footholdAngularVelocity = physics.BodyAngularVelocity(footholdID);
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
			/// [EN] In the air the horizontal velocity and the turn carried so far are kept as they are; the step's damping already slows them at the same rate as the body's own, so landing back on the same foothold does not give them twice.
			/// [JP] 空中では、それまで受けていた水平の速度と回転をそのまま残す。ステップの減衰がボディ自身と同じ割合で既に弱めているので、同じ足場に戻っても二重に受けない。
			/// [EN] The vertical part becomes the body's own, so gravity acts on the whole of it.
			/// [JP] 垂直の分はボディ自身の速度になり、重力がその全体に働く。
			nextLinearVelocity = Vector3(inheritedLinearVelocity_.x, 0.0f, inheritedLinearVelocity_.z);
			nextAngularVelocity = inheritedAngularVelocity_;
		}

		/// [EN] What the foothold gave in the last step is swapped for what it gives now, so only the change reaches the body.
		/// [JP] 前のステップで足場から受けた分を今受ける分に入れ替え、ボディには変化分だけを届ける。
		physics.BodyLinearVelocity(bodyID_, physics.BodyLinearVelocity(bodyID_) - inheritedLinearVelocity_ + nextLinearVelocity);
		physics.BodyAngularVelocity(bodyID_, physics.BodyAngularVelocity(bodyID_) - inheritedAngularVelocity_ + nextAngularVelocity);

		inheritedLinearVelocity_ = nextLinearVelocity;
		inheritedAngularVelocity_ = nextAngularVelocity;
	}
}
