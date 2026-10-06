#include <PhysicsEngine/CharacterController/CharacterController.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/JoltPhysics/JoltLayerdef.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the virtual character from the actor transform and settings.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アクターの変換と設定から仮想キャラクターを生成する。
	*/
	void CharacterController::OnAwake()
	{
		/// [EN] Build the Jolt character description from the component and actor state.
		/// [JP] コンポーネントとアクターの状態から Jolt キャラクター設定を構築する。
		Actor actor = GetActor();

		const Position* position = actor.GetComponent<Position>();
		const Rotation* rotation = actor.GetComponent<Rotation>();

		CharacterDesc desc;
		desc.position_ = position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f);
		desc.rotation_ = rotation ? Transform::Quat(*rotation) : Quaternion::Identity;
		desc.radius_ = radius_;
		desc.height_ = height_;
		desc.maxSlopeAngle_ = ToRadians(maxSlopeAngle_);
		desc.mass_ = mass_;
		desc.maxStrength_ = pushForce_;
		desc.layer_ = Layers::Pack(Layers::DYNAMIC, actor.Layer());
		desc.userData_ = actor.GetEntity().GetID();

		character_ = actor.GetPhysics().CreateCharacter(desc);

		/// [EN] Forward character contacts to the owning entity in this World.
		/// [JP] キャラクターの接触を、この World 内の所有エンティティへ転送する。
		characterContactListener_ = new JoltCharacterContactListener();
		characterContactListener_->SetTarget(&actor.GetWorld(), desc.userData_);
		character_->SetListener(characterContactListener_.GetPtr());
	}

	/**
	* [EN]
	* Advances character movement and writes the resulting transform to the actor.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* キャラクター移動を進め、結果の変換をアクターへ書き戻す。
	*/
	void CharacterController::OnFixedTick(Float elapsedTime)
	{
		if (!character_)
		{
			return;
		}

		/// [EN] A bound custom callback fully replaces the built-in movement path.
		/// [JP] カスタムコールバックが設定されている場合は、標準移動処理を完全に置き換える。
		if (onCustomMove_.Bound())
		{
			/// [EN] The custom callback owns the velocity, so foothold tracking restarts once it is unbound.
			/// [JP] 速度はカスタムコールバックが管理するため、解除後は足場の追従を最初からやり直す。
			foothold_.bodyID_ = JPH::BodyID();
			inheritedVelocity_ = Vector3(0.0f, 0.0f, 0.0f);

			onCustomMove_.Execute(elapsedTime);
			return;
		}

		Actor actor = GetActor();

		/// [EN] Change capsule height only when the requested crouch state changes.
		/// [JP] 要求されたしゃがみ状態が変わったときだけカプセル高を変更する。
		if (crouch_ && !isCrouched_)
		{
			isCrouched_ = actor.GetPhysics().CharacterHeight(character_.GetPtr(), crouchHeight_, radius_);
		}
		else if (!crouch_ && isCrouched_)
		{
			isCrouched_ = !actor.GetPhysics().CharacterHeight(character_.GetPtr(), height_, radius_);
		}

		/// [EN] Work in velocity relative to the foothold so input acceleration never acts on the carried velocity.
		/// [JP] 入力による加減速が引き継いだ速度にかからないよう、足場に対する相対速度で計算する。
		JPH::Vec3 currentVelocity = character_->GetLinearVelocity();
		Vector3 horizontalVelocity(currentVelocity.GetX() - inheritedVelocity_.x, 0.0f, currentVelocity.GetZ() - inheritedVelocity_.z);
		Float verticalVelocity = currentVelocity.GetY() - inheritedVelocity_.y;

		/// [EN] Normalize movement input while preserving values below full magnitude.
		/// [JP] 最大未満の入力強度を保持しながら移動入力を正規化する。
		Vector3 inputDirection = moveDirection_;
		Float inputLength = inputDirection.Length();
		if (inputLength > 0.0001f)
		{
			inputDirection /= inputLength;
		}
		else
		{
			inputDirection = Vector3(0.0f, 0.0f, 0.0f);
			inputLength = 0.0f;
		}

		Vector3 targetHorizontalVelocity = inputDirection * (Min(inputLength, 1.0f) * maxMoveSpeed_);
		Float rate = (inputLength > 0.0001f) ? acceleration_ : deceleration_;

		/// [EN] Approach the target horizontal velocity at the configured rate.
		/// [JP] 設定された変化率で目標水平速度へ近づける。
		Vector3 horizontalDelta = targetHorizontalVelocity - horizontalVelocity;
		Float horizontalDeltaLength = horizontalDelta.Length();
		Float maxDelta = rate * elapsedTime;
		if (horizontalDeltaLength > maxDelta)
		{
			horizontalVelocity += horizontalDelta * (maxDelta / horizontalDeltaLength);
		}
		else
		{
			horizontalVelocity = targetHorizontalVelocity;
		}

		Bool grounded = character_->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;

		/// [EN] Air damping slows both the character's own motion and the inertia carried from the foothold.
		/// [JP] 空気抵抗は、キャラクター自身の動きと足場から引き継いだ慣性の両方を減速させる。
		if (!grounded)
		{
			Float airDamping = Max(0.0f, 1.0f - airDrag_ * elapsedTime);
			horizontalVelocity *= airDamping;
			inheritedVelocity_.x *= airDamping;
			inheritedVelocity_.z *= airDamping;
		}

		if (grounded && verticalVelocity <= 0.0f)
		{
			/// [EN] While standing, the foothold supplies the vertical motion, so the character's own vertical velocity restarts from rest.
			/// [JP] 立っている間は足場が上下の動きを担うため、キャラクター自身の上下速度は静止から計算し直す。
			verticalVelocity = 0.0f;

			Vector3 footholdPosition;
			Quaternion footholdRotation;
			if (!foothold_.bodyID_.IsInvalid() && actor.GetPhysics().BodyExists(foothold_.bodyID_) && elapsedTime > 0.0f)
			{
				actor.GetPhysics().BodyTransform(foothold_.bodyID_, footholdPosition, footholdRotation);

				/// [EN] Carry the recorded local position along with the body's current transform; the exact displacement over this step becomes the inherited velocity, so no drift accumulates.
				/// [JP] 記録したローカル位置をボディの現在の変換で運ぶ。このステップでの正確な移動量を引き継ぐ速度にするため、ずれが蓄積しない。
				JPH::RVec3 characterPosition = character_->GetPosition();
				Vector3 carriedPosition = Vector3::Transform(foothold_.position_, footholdRotation) + footholdPosition;
				Vector3 displacement = carriedPosition - Vector3(static_cast<Float>(characterPosition.GetX()), static_cast<Float>(characterPosition.GetY()), static_cast<Float>(characterPosition.GetZ()));
				inheritedVelocity_ = displacement / elapsedTime;

				/// [EN] World-space rotation of the body since the record: the recorded rotation undone, followed by the current one.
				/// [JP] 記録時からのボディのワールド空間での回転。記録時の回転を戻し、現在の回転を続けて適用したもの。
				Quaternion inverseRotation;
				foothold_.rotation_.Inverse(inverseRotation);
				Quaternion deltaRotation = inverseRotation * footholdRotation;

				/// [EN] The shorter of the two equivalent quaternions keeps the extracted angle within half a turn.
				/// [JP] 等価な2つのクォータニオンのうち短い方を使い、取り出す角度を半回転以内に収める。
				if (deltaRotation.w < 0.0f)
				{
					deltaRotation = -deltaRotation;
				}

				/// [EN] Only the twist about the vertical axis turns the character; a tilting foothold must not tip it over.
				/// [JP] キャラクターを回すのは垂直軸まわりのねじれ成分だけ。足場が傾いてもキャラクターは倒さない。
				Float yawDelta = 2.0f * Atan2(deltaRotation.y, deltaRotation.w);
				Quaternion yawRotation = Quaternion::CreateFromAxisAngle(Vector3::UnitY, yawDelta);

				JPH::Quat currentJoltRotation = character_->GetRotation();
				Quaternion turnedRotation = Quaternion(currentJoltRotation.GetX(), currentJoltRotation.GetY(), currentJoltRotation.GetZ(), currentJoltRotation.GetW()) * yawRotation;
				character_->SetRotation(JPH::Quat(turnedRotation.x, turnedRotation.y, turnedRotation.z, turnedRotation.w));

				/// [EN] Turn the requested facing too, so the rotation step below does not undo the foothold's turn.
				/// [JP] 下の回転処理が足場の回転を打ち消さないよう、要求中の正面方向も一緒に回す。
				forwardDirection_ = Vector3::Transform(forwardDirection_, yawRotation);
			}
			else
			{
				/// [EN] On the first step on a new foothold there is no record yet, so its contact-point velocity stands in.
				/// [JP] 新しい足場に乗った最初のステップは記録がないため、接地点の速度で代用する。
				JPH::Vec3 groundVelocity = character_->GetGroundVelocity();
				inheritedVelocity_ = Vector3(groundVelocity.GetX(), groundVelocity.GetY(), groundVelocity.GetZ());
			}
		}
		else
		{
			/// [EN] Once off the foothold, its vertical velocity becomes part of the character's own so gravity acts on the total.
			/// [JP] 足場を離れたら、足場の上下速度をキャラクター自身の速度へ移し、重力が合計に対して働くようにする。
			verticalVelocity += inheritedVelocity_.y;
			inheritedVelocity_.y = 0.0f;
		}

		Vector3 gravity = actor.GetPhysics().Gravity();
		verticalVelocity += gravity.y * gravityScale_ * elapsedTime;
		verticalVelocity = Max(verticalVelocity, -maxFallSpeed_);

		character_->SetLinearVelocity(JPH::Vec3(horizontalVelocity.x + inheritedVelocity_.x, verticalVelocity + inheritedVelocity_.y, horizontalVelocity.z + inheritedVelocity_.z));

		/// [EN] Rotate toward the requested horizontal facing direction at a limited angular speed.
		/// [JP] 角速度を制限しながら、要求された水平方向へ回転する。
		Vector3 forwardXZ(forwardDirection_.x, 0.0f, forwardDirection_.z);
		Float forwardLength = forwardXZ.Length();
		if (forwardLength > 0.0001f)
		{
			forwardXZ /= forwardLength;

			Quaternion targetRotation = Quaternion::CreateFromYawPitchRoll(Atan2(forwardXZ.x, forwardXZ.z), 0.0f, 0.0f);

			JPH::Quat currentJoltRotation = character_->GetRotation();
			Quaternion currentRotation(currentJoltRotation.GetX(), currentJoltRotation.GetY(), currentJoltRotation.GetZ(), currentJoltRotation.GetW());

			Float dot = Clamp(Abs(currentRotation.Dot(targetRotation)), -1.0f, 1.0f);
			Float angleBetween = 2.0f * Acos(dot);
			Float maxAngle = ToRadians(turnSpeed_) * elapsedTime;

			Quaternion newRotation = (angleBetween <= maxAngle) ? targetRotation : Quaternion::Slerp(currentRotation, targetRotation, maxAngle / angleBetween);

			character_->SetRotation(JPH::Quat(newRotation.x, newRotation.y, newRotation.z, newRotation.w));
		}

		/// [EN] Each contacted body dispatches at most one enter or stay per update.
		/// [JP] 接触先ボディごとの Enter か Stay は、更新1回につき最大1回。
		characterContactListener_->Begin();
		actor.GetPhysics().UpdateCharacter(character_.GetPtr(), elapsedTime, ToRadians(maxSlopeAngle_), maxStepHeight_);

		JPH::RVec3 outPosition = character_->GetPosition();
		JPH::Quat outRotation = character_->GetRotation();

		/// [EN] Record where the character now stands on its foothold, in the foothold's local space, for the next step to carry it from.
		/// [JP] 次のステップで運ぶ基準として、キャラクターが今足場のどこに立っているかを足場のローカル空間で記録する。
		JPH::BodyID groundBodyID = character_->GetGroundBodyID();
		if (character_->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround && actor.GetPhysics().BodyExists(groundBodyID))
		{
			Vector3 footholdPosition;
			Quaternion footholdRotation;
			actor.GetPhysics().BodyTransform(groundBodyID, footholdPosition, footholdRotation);

			Quaternion inverseRotation;
			footholdRotation.Inverse(inverseRotation);

			Vector3 characterPosition = { static_cast<Float>(outPosition.GetX()), static_cast<Float>(outPosition.GetY()), static_cast<Float>(outPosition.GetZ()) };

			foothold_.bodyID_ = groundBodyID;
			foothold_.position_ = Vector3::Transform(characterPosition - footholdPosition, inverseRotation);
			foothold_.rotation_ = footholdRotation;
		}
		else
		{
			foothold_.bodyID_ = JPH::BodyID();
		}

		/// [EN] Synchronize the actor transform components with the simulated character pose.
		/// [JP] シミュレーション後のキャラクター姿勢をアクターの変換コンポーネントへ同期する。

		World& world = actor.GetWorld();
		Entity entity = actor.GetEntity();

		Position* position = world.GetComponent<Position>(entity);
		if (position)
		{
			position->x_ = static_cast<Float>(outPosition.GetX());
			position->y_ = static_cast<Float>(outPosition.GetY());
			position->z_ = static_cast<Float>(outPosition.GetZ());
		}

		Rotation* rotation = world.GetComponent<Rotation>(entity);
		if (rotation)
		{
			rotation->x_ = outRotation.GetX();
			rotation->y_ = outRotation.GetY();
			rotation->z_ = outRotation.GetZ();
			rotation->w_ = outRotation.GetW();
		}
	}

	/**
	* [EN]
	* Destroys the virtual character and releases its contact listener.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 仮想キャラクターを破棄し、接触リスナーを解放する。
	*/
	void CharacterController::OnDestroy()
	{
		GetActor().GetPhysics().DestroyCharacter(character_);
		characterContactListener_ = nullptr;
	}

	/**
	* [EN]
	* Sets the requested horizontal movement direction and magnitude.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 要求する水平移動の方向と大きさを設定する。
	*/
	void CharacterController::MoveDirection(const Vector3& moveDirection)
	{
		moveDirection_ = moveDirection;
	}

	/**
	* [EN]
	* Returns the requested movement direction and magnitude.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 要求中の移動方向と大きさを返す。
	*/
	const Vector3& CharacterController::MoveDirection()const
	{
		return moveDirection_;
	}

	/**
	* [EN]
	* Sets the horizontal direction the character should face.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* キャラクターが向く水平方向を設定する。
	*/
	void CharacterController::ForwardDirection(const Vector3& forwardDirection)
	{
		forwardDirection_ = forwardDirection;
	}

	/**
	* [EN]
	* Returns the requested facing direction.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 要求中の正面方向を返す。
	*/
	const Vector3& CharacterController::ForwardDirection()const
	{
		return forwardDirection_;
	}

	/**
	* [EN]
	* Applies the configured upward jump velocity while on the ground.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 接地中に設定された上向きジャンプ速度を適用する。
	*/
	void CharacterController::Jump()
	{
		if (!character_)
		{
			return;
		}

		/// [EN] The jump power is relative to the foothold, so jumping from a rising foothold goes higher.
		/// [JP] ジャンプ力は足場に対する相対値なので、上昇中の足場から跳ぶとその分高く跳ぶ。
		JPH::Vec3 velocity = character_->GetLinearVelocity();
		character_->SetLinearVelocity(JPH::Vec3(velocity.GetX(), inheritedVelocity_.y + jumpPower_, velocity.GetZ()));
	}

	/**
	* [EN]
	* Moves the character immediately and synchronizes the actor position.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* キャラクターを即座に移動し、アクターの位置を同期する。
	*/
	void CharacterController::Teleport(const Vector3& position)
	{
		if (!character_)
		{
			return;
		}

		character_->SetPosition(JPH::RVec3(position.x, position.y, position.z));

		/// [EN] A teleport leaves the foothold behind, so neither its record nor its velocity carries over.
		/// [JP] テレポートすると足場から離れるため、足場の記録も速度も引き継がない。
		foothold_.bodyID_ = JPH::BodyID();
		inheritedVelocity_ = Vector3(0.0f, 0.0f, 0.0f);

		Actor actor = GetActor();
		actor.GetPhysics().RefreshCharacter(character_.GetPtr());

		World& world = actor.GetWorld();
		Entity entity = actor.GetEntity();

		Position* positionComponent = world.GetComponent<Position>(entity);
		if (positionComponent)
		{
			Transform::Vector(*positionComponent, position);
		}
	}

	/**
	* [EN]
	* Reports whether Jolt classifies the character as on the ground.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt がキャラクターを接地状態と判定しているかを返す。
	*/
	Bool CharacterController::OnGround()const
	{
		if (!character_)
		{
			return false;
		}

		return character_->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;
	}

	/**
	* [EN]
	* Reports whether an active contact has a wall-facing normal.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 有効な接触に壁方向の法線があるかを返す。
	*/
	Bool CharacterController::OnWall()const
	{
		if (!character_)
		{
			return false;
		}

		for (const JPH::CharacterContact& contact : character_->GetActiveContacts())
		{
			if (contact.mHadCollision && Abs(contact.mContactNormal.Dot(JPH::Vec3::sAxisY())) < wallNormalDotLimit_)
			{
				return true;
			}
		}

		return false;
	}

	/**
	* [EN]
	* Reports whether an active contact faces downward from a ceiling.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 有効な接触に天井から下向きの法線があるかを返す。
	*/
	Bool CharacterController::OnCeiling()const
	{
		if (!character_)
		{
			return false;
		}

		for (const JPH::CharacterContact& contact : character_->GetActiveContacts())
		{
			if (contact.mHadCollision && contact.mContactNormal.Dot(JPH::Vec3::sAxisY()) < -wallNormalDotLimit_)
			{
				return true;
			}
		}

		return false;
	}

	/**
	* [EN]
	* Reports whether the grounded surface is inclined.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 接地面が傾斜しているかを返す。
	*/
	Bool CharacterController::OnSlope()const
	{
		if (!character_ || character_->GetGroundState() != JPH::CharacterBase::EGroundState::OnGround)
		{
			return false;
		}

		return character_->GetGroundNormal().Dot(JPH::Vec3::sAxisY()) < 0.999f;
	}

	/**
	* [EN]
	* Returns the current ground-contact normal.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の接地法線を返す。
	*/
	Vector3 CharacterController::GroundNormal()const
	{
		if (!character_)
		{
			return Vector3(0.0f, 1.0f, 0.0f);
		}

		JPH::Vec3 normal = character_->GetGroundNormal();
		return Vector3(normal.GetX(), normal.GetY(), normal.GetZ());
	}

	/**
	* [EN]
	* Returns the first active wall-contact normal.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 最初に見つかった有効な壁接触法線を返す。
	*/
	Vector3 CharacterController::WallNormal()const
	{
		if (character_)
		{
			for (const JPH::CharacterContact& contact : character_->GetActiveContacts())
			{
				if (contact.mHadCollision && Abs(contact.mContactNormal.Dot(JPH::Vec3::sAxisY())) < wallNormalDotLimit_)
				{
					return Vector3(contact.mContactNormal.GetX(), contact.mContactNormal.GetY(), contact.mContactNormal.GetZ());
				}
			}
		}

		return Vector3(0.0f, 0.0f, 0.0f);
	}

	/**
	* [EN]
	* Returns the first active ceiling-contact normal.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 最初に見つかった有効な天井接触法線を返す。
	*/
	Vector3 CharacterController::CeilingNormal()const
	{
		if (character_)
		{
			for (const JPH::CharacterContact& contact : character_->GetActiveContacts())
			{
				if (contact.mHadCollision && contact.mContactNormal.Dot(JPH::Vec3::sAxisY()) < -wallNormalDotLimit_)
				{
					return Vector3(contact.mContactNormal.GetX(), contact.mContactNormal.GetY(), contact.mContactNormal.GetZ());
				}
			}
		}

		return Vector3(0.0f, 0.0f, 0.0f);
	}

	/**
	* [EN]
	* Reports whether the crouched capsule is currently active.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* しゃがみ用カプセルが現在有効かを返す。
	*/
	Bool CharacterController::Crouching()const
	{
		return isCrouched_;
	}

	/**
	* [EN]
	* Reports whether an unsupported character is moving downward.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 支持されていないキャラクターが下降中かを返す。
	*/
	Bool CharacterController::Falling()const
	{
		if (!character_ || character_->IsSupported())
		{
			return false;
		}

		return character_->GetLinearVelocity().GetY() < 0.0f;
	}

	/**
	* [EN]
	* Reports whether the character ground state is in air.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* キャラクターの接地状態が空中かを返す。
	*/
	Bool CharacterController::Flying()const
	{
		if (!character_)
		{
			return false;
		}

		return character_->GetGroundState() == JPH::CharacterBase::EGroundState::InAir;
	}

	/**
	* [EN]
	* Reports whether the character is supported by a surface.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* キャラクターが面に支持されているかを返す。
	*/
	Bool CharacterController::Grounded()const
	{
		if (!character_)
		{
			return false;
		}

		return character_->IsSupported();
	}

	/**
	* [EN]
	* Reports whether an unsupported character is moving upward.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 支持されていないキャラクターが上昇中かを返す。
	*/
	Bool CharacterController::Jumping()const
	{
		if (!character_ || character_->IsSupported())
		{
			return false;
		}

		return character_->GetLinearVelocity().GetY() > 0.0f;
	}

	/**
	* [EN]
	* Reports whether a supported character has horizontal velocity.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 支持されたキャラクターに水平速度があるかを返す。
	*/
	Bool CharacterController::Running()const
	{
		if (!character_ || !character_->IsSupported())
		{
			return false;
		}

		JPH::Vec3 velocity = character_->GetLinearVelocity();
		Float horizontalSpeedSquared = velocity.GetX() * velocity.GetX() + velocity.GetZ() * velocity.GetZ();
		return horizontalSpeedSquared > 0.0001f;
	}

	/**
	* [EN]
	* Reports whether a supported character has no horizontal velocity.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 支持されたキャラクターに水平速度がないかを返す。
	*/
	Bool CharacterController::Stopped()const
	{
		if (!character_ || !character_->IsSupported())
		{
			return false;
		}

		JPH::Vec3 velocity = character_->GetLinearVelocity();
		Float horizontalSpeedSquared = velocity.GetX() * velocity.GetX() + velocity.GetZ() * velocity.GetZ();
		return horizontalSpeedSquared <= 0.0001f;
	}
}
