#include <PhysicsEngine/JoltPhysics/JoltConfig.h>
#include <External/JoltPhysics/Jolt/Jolt.h>
#include <External/JoltPhysics/Jolt/Physics/Character/CharacterBase.h>
#include <External/JoltPhysics/Jolt/Physics/Character/CharacterVirtual.h>
#include <PhysicsEngine/CharacterController/CharacterController.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/JoltPhysics/JoltCharacterContactListener.h>
#include <PhysicsEngine/JoltPhysics/JoltLayerdef.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>

namespace SeedCore
{
	/**
	* [JP]
	* キャラクターと、ぶつかりを知らせる係を作る。
	*
	* @brief キャラクターを作る
	*
	* [EN]
	* Creates the character and the part that reports its hits.
	*
	* @brief Creates the character
	*/
	void CharacterController::OnAwake()
	{
		Actor actor = GetActor();

		/// [JP] 親から見た位置と向きを、世界での位置と向きに直す
		/// [EN] Turn the parent-relative position and facing into world ones
		const Position* position = actor.GetComponent<Position>();
		const Rotation* rotation = actor.GetComponent<Rotation>();

		Actor parentActor = actor.Parent();
		Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
		Vector3 parentScale;
		Quaternion parentRotation;
		Vector3 parentTranslation;
		parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);

		/// [JP] 設定をまとめて、キャラクターを作る
		/// [EN] Gather the settings and create the character
		CharacterDesc desc;
		desc.position_ = Vector3::TransformPosition(position ? Vector3(position->x_, position->y_, position->z_) : Vector3(0.0f, 0.0f, 0.0f), parentMatrix);
		desc.rotation_ = (rotation ? Transform::Quat(*rotation) : Quaternion::Identity) * parentRotation;
		desc.radius_ = radius_;
		desc.height_ = height_;
		desc.maxSlopeAngle_ = ToRadians(maxSlopeAngle_);
		desc.mass_ = mass_;
		desc.maxStrength_ = pushForce_;
		desc.layer_ = Layers::Pack(Layers::DYNAMIC, actor.Layer());
		desc.userData_ = ActorAccessor::GetEntity(actor).GetID();

		character_ = actor.GetService<Physics>()->CreateCharacter(desc);

		/// [JP] ぶつかりを知らせる係を作って、キャラクターにつなぐ
		/// [EN] Create the hit reporter and connect it to the character
		JoltCharacterContactListener* listener = new JoltCharacterContactListener(actor.GetWorld(), desc.userData_);
		characterContactListener_ = listener;
		listener->Attach(character_);
	}

	/**
	* [JP]
	* 入力に合わせて速さと向きを決め、キャラクターを1回分動かして、アクターに写す。
	*
	* @brief キャラクターを動かす
	* @param elapsedTime 前の物理の更新からの時間
	* @note  onCustomMove_ が登録されていれば、そちらに任せる
	*
	* [EN]
	* Sets speed and facing from the input, moves the character one step and copies it to the actor.
	*
	* @brief Moves the character
	* @param elapsedTime Time since the last physics update
	* @note  If onCustomMove_ is set, movement is left to it
	*/
	void CharacterController::OnFixedTick(Float elapsedTime)
	{
		if (!character_)
		{
			return;
		}

		/// [JP] 動きを自分で書く登録があれば、そちらに任せる
		/// [EN] If custom movement is set, leave it to that
		if (onCustomMove_.Bound())
		{
			foothold_.bodyID_ = ScInvalidBodyID;
			inheritedVelocity_ = Vector3(0.0f, 0.0f, 0.0f);

			onCustomMove_.Execute(elapsedTime);
			return;
		}

		Actor actor = GetActor();

		/// [JP] しゃがむ・立つを切り替える。周りにぶつかるなら変えない
		/// [EN] Switch between crouching and standing, unless it would hit something
		if (crouch_ && !isCrouched_)
		{
			isCrouched_ = actor.GetService<Physics>()->CharacterHeight(character_, crouchHeight_, radius_);
		}
		else if (!crouch_ && isCrouched_)
		{
			isCrouched_ = !actor.GetService<Physics>()->CharacterHeight(character_, height_, radius_);
		}

		/// [JP] 今の速さから、足場の分を引いた自分の速さを出す
		/// [EN] Take the foothold part out of the current speed to get its own speed
		JPH::Vec3 currentVelocity = static_cast<JPH::CharacterVirtual*>(character_)->GetLinearVelocity();
		Vector3 horizontalVelocity(currentVelocity.GetX() - inheritedVelocity_.x, 0.0f, currentVelocity.GetZ() - inheritedVelocity_.z);
		Float verticalVelocity = currentVelocity.GetY() - inheritedVelocity_.y;

		/// [JP] 入力の向きと強さを出す
		/// [EN] Work out the input direction and strength
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

		/// [JP] 目標の速さに向けて、加速か減速の早さで近づける
		/// [EN] Move toward the target speed at the speed-up or slow-down rate
		Vector3 targetHorizontalVelocity = inputDirection * (Min(inputLength, 1.0f) * maxMoveSpeed_);
		Float rate = (inputLength > 0.0001f) ? acceleration_ : deceleration_;

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

		/// [JP] 空中では横の動きを弱めていく
		/// [EN] In the air, let sideways movement fade
		Bool grounded = static_cast<JPH::CharacterVirtual*>(character_)->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;

		if (!grounded)
		{
			Float airDamping = Max(0.0f, 1.0f - airDrag_ * elapsedTime);
			horizontalVelocity *= airDamping;
			inheritedVelocity_.x *= airDamping;
			inheritedVelocity_.z *= airDamping;
		}

		/// [JP] 地面にいるときは、足場の動きを受け取る
		/// [EN] On the ground, take on the foothold's motion
		if (grounded && verticalVelocity <= 0.0f)
		{
			verticalVelocity = 0.0f;

			/// [JP] 足場が動いた分を速さにし、回った分だけキャラクターも回す
			/// [EN] Turn the foothold's movement into speed and turn the character with it
			if (foothold_.bodyID_ != ScInvalidBodyID && actor.GetService<Physics>()->BodyExists(foothold_.bodyID_) && elapsedTime > 0.0f)
			{
				Matrix footholdMatrix = actor.GetService<Physics>()->BodyTransform(foothold_.bodyID_);
				Vector3 footholdPosition = footholdMatrix.Translation();
				Quaternion footholdRotation = Quaternion::CreateFromRotationMatrix(footholdMatrix);

				JPH::RVec3 characterPosition = static_cast<JPH::CharacterVirtual*>(character_)->GetPosition();
				Vector3 carriedPosition = Vector3::Transform(foothold_.position_, footholdRotation) + footholdPosition;
				Vector3 displacement = carriedPosition - Vector3(static_cast<Float>(characterPosition.GetX()), static_cast<Float>(characterPosition.GetY()), static_cast<Float>(characterPosition.GetZ()));
				inheritedVelocity_ = displacement / elapsedTime;

				Quaternion inverseRotation;
				inverseRotation = foothold_.rotation_.Inverse();
				Quaternion deltaRotation = inverseRotation * footholdRotation;

				if (deltaRotation.w < 0.0f)
				{
					deltaRotation = -deltaRotation;
				}

				Float yawDelta = 2.0f * Atan2(deltaRotation.y, deltaRotation.w);
				Quaternion yawRotation = Quaternion::CreateFromAxisAngle(Vector3::UnitY, yawDelta);

				JPH::Quat currentJoltRotation = static_cast<JPH::CharacterVirtual*>(character_)->GetRotation();
				Quaternion turnedRotation = Quaternion(currentJoltRotation.GetX(), currentJoltRotation.GetY(), currentJoltRotation.GetZ(), currentJoltRotation.GetW()) * yawRotation;
				static_cast<JPH::CharacterVirtual*>(character_)->SetRotation(JPH::Quat(turnedRotation.x, turnedRotation.y, turnedRotation.z, turnedRotation.w));

				forwardDirection_ = Vector3::Transform(forwardDirection_, yawRotation);
			}
			else
			{
				/// [JP] 前の足場がなければ、地面の速さをそのまま受け取る
				/// [EN] Without the earlier foothold, just take the ground's speed
				JPH::Vec3 groundVelocity = static_cast<JPH::CharacterVirtual*>(character_)->GetGroundVelocity();
				inheritedVelocity_ = Vector3(groundVelocity.GetX(), groundVelocity.GetY(), groundVelocity.GetZ());
			}
		}
		else
		{
			/// [JP] 空中では、上下の受け取った速さを自分の速さに移す
			/// [EN] In the air, move the inherited up-down speed into its own speed
			verticalVelocity += inheritedVelocity_.y;
			inheritedVelocity_.y = 0.0f;
		}

		/// [JP] 重力をかけ、落ちる速さに上限をつける
		/// [EN] Apply gravity and cap the falling speed
		Vector3 gravity = actor.GetService<Physics>()->Gravity();
		verticalVelocity += gravity.y * gravityScale_ * elapsedTime;
		verticalVelocity = Max(verticalVelocity, -maxFallSpeed_);

		static_cast<JPH::CharacterVirtual*>(character_)->SetLinearVelocity(JPH::Vec3(horizontalVelocity.x + inheritedVelocity_.x, verticalVelocity + inheritedVelocity_.y, horizontalVelocity.z + inheritedVelocity_.z));

		/// [JP] 向きたい方向へ、決めた速さで少しずつ向く
		/// [EN] Turn toward the facing direction at the set turn speed
		Vector3 forwardXZ(forwardDirection_.x, 0.0f, forwardDirection_.z);
		Float forwardLength = forwardXZ.Length();
		if (forwardLength > 0.0001f)
		{
			forwardXZ /= forwardLength;

			Quaternion targetRotation = Quaternion::CreateFromYawPitchRoll(Atan2(forwardXZ.x, forwardXZ.z), 0.0f, 0.0f);

			JPH::Quat currentJoltRotation = static_cast<JPH::CharacterVirtual*>(character_)->GetRotation();
			Quaternion currentRotation(currentJoltRotation.GetX(), currentJoltRotation.GetY(), currentJoltRotation.GetZ(), currentJoltRotation.GetW());

			Float dot = Clamp(Abs(currentRotation.Dot(targetRotation)), -1.0f, 1.0f);
			Float angleBetween = 2.0f * Acos(dot);
			Float maxAngle = ToRadians(turnSpeed_) * elapsedTime;

			Quaternion newRotation = (angleBetween <= maxAngle) ? targetRotation : Quaternion::Slerp(currentRotation, targetRotation, maxAngle / angleBetween);

			static_cast<JPH::CharacterVirtual*>(character_)->SetRotation(JPH::Quat(newRotation.x, newRotation.y, newRotation.z, newRotation.w));
		}

		/// [JP] ぶつかりの記録を空にしてから、1回分動かす
		/// [EN] Clear the hit record, then move one step
		static_cast<JoltCharacterContactListener*>(characterContactListener_)->Begin();
		actor.GetService<Physics>()->UpdateCharacter(character_, elapsedTime, ToRadians(maxSlopeAngle_), maxStepHeight_);

		JPH::RVec3 outPosition = static_cast<JPH::CharacterVirtual*>(character_)->GetPosition();
		JPH::Quat outRotation = static_cast<JPH::CharacterVirtual*>(character_)->GetRotation();

		/// [JP] 地面に立っていれば、次のために足場との位置関係を覚える
		/// [EN] If standing on ground, remember where it is on the foothold for next time
		ScBodyID groundBodyID = static_cast<JPH::CharacterVirtual*>(character_)->GetGroundBodyID().GetIndexAndSequenceNumber();
		if (static_cast<JPH::CharacterVirtual*>(character_)->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround && actor.GetService<Physics>()->BodyExists(groundBodyID))
		{
			Matrix footholdMatrix = actor.GetService<Physics>()->BodyTransform(groundBodyID);
			Vector3 footholdPosition = footholdMatrix.Translation();
			Quaternion footholdRotation = Quaternion::CreateFromRotationMatrix(footholdMatrix);

			Quaternion inverseRotation;
			inverseRotation = footholdRotation.Inverse();

			Vector3 characterPosition = { static_cast<Float>(outPosition.GetX()), static_cast<Float>(outPosition.GetY()), static_cast<Float>(outPosition.GetZ()) };

			foothold_.bodyID_ = groundBodyID;
			foothold_.position_ = Vector3::Transform(characterPosition - footholdPosition, inverseRotation);
			foothold_.rotation_ = footholdRotation;
		}
		else
		{
			foothold_.bodyID_ = ScInvalidBodyID;
		}

		/// [JP] 動いた結果を、親から見た位置と向きに直してアクターに写す
		/// [EN] Copy the result to the actor, relative to its parent
		World& world = actor.GetWorld();
		Entity entity = ActorAccessor::GetEntity(actor);

		Actor parentActor = actor.Parent();
		Matrix parentMatrix = parentActor ? parentActor.WorldMatrix() : Matrix::Identity;
		Vector3 parentScale;
		Quaternion parentRotation;
		Vector3 parentTranslation;
		parentMatrix.Decompose(parentTranslation, parentRotation, parentScale);

		Quaternion inverseParentRotation;
		inverseParentRotation = parentRotation.Inverse();

		Position* position = world.GetComponent<Position>(entity);
		if (position)
		{
			Transform::Vector(*position, Vector3::TransformPosition(Vector3(static_cast<Float>(outPosition.GetX()), static_cast<Float>(outPosition.GetY()), static_cast<Float>(outPosition.GetZ())), parentMatrix.Invert()));
		}

		Rotation* rotation = world.GetComponent<Rotation>(entity);
		if (rotation)
		{
			Transform::Quat(*rotation, Quaternion(outRotation.GetX(), outRotation.GetY(), outRotation.GetZ(), outRotation.GetW()) * inverseParentRotation);
		}
	}

	/**
	* [JP]
	* キャラクターと、ぶつかりを知らせる係を消す。
	*
	* @brief キャラクターを消す
	*
	* [EN]
	* Removes the character and the hit reporter.
	*
	* @brief Removes the character
	*/
	void CharacterController::OnDestroy()
	{
		/// [JP] キャラクターと、ぶつかりを知らせる係を消す
		/// [EN] Remove the character and the hit reporter
		GetActor().GetService<Physics>()->DestroyCharacter(character_);
		if (characterContactListener_)
		{
			delete static_cast<JoltCharacterContactListener*>(characterContactListener_);
			characterContactListener_ = nullptr;
		}
	}

	/**
	* [JP]
	* 設定が変わったので、キャラクターを作り直す。
	*
	* @brief 作り直す
	*
	* [EN]
	* The settings changed, so the character is made again.
	*
	* @brief Remakes the character
	*/
	void CharacterController::OnRevision()
	{
		if (!character_)
		{
			return;
		}

		OnDestroy();
		OnAwake();
	}

	/**
	* [JP]
	* キャラクターを作り直す。
	*
	* @brief 作り直す
	*
	* [EN]
	* Remakes the character.
	*
	* @brief Remakes the character
	*/
	void CharacterController::Dispatch()
	{
		OnRevision();
	}

	/**
	* [JP]
	* 動きたい向きを決める。長さ 1 で一番速くなる。
	*
	* @brief 動く向きを決める
	* @param moveDirection 動きたい向き
	*
	* [EN]
	* Sets the direction to move in; a length of 1 means full speed.
	*
	* @brief Sets the move direction
	* @param moveDirection The direction to move in
	*/
	void CharacterController::MoveDirection(const Vector3& moveDirection)
	{
		moveDirection_ = moveDirection;
	}

	/**
	* [JP]
	* 動きたい向きを返す。
	*
	* @brief  動く向きを返す
	* @return 動きたい向き
	*
	* [EN]
	* Returns the direction to move in.
	*
	* @brief  Returns the move direction
	* @return The direction to move in
	*/
	const Vector3& CharacterController::MoveDirection()const
	{
		return moveDirection_;
	}

	/**
	* [JP]
	* 向きたい方向を決める。少しずつその方向を向く。
	*
	* @brief 向く方向を決める
	* @param forwardDirection 向きたい方向
	*
	* [EN]
	* Sets the direction to face; it turns toward it bit by bit.
	*
	* @brief Sets the facing direction
	* @param forwardDirection The direction to face
	*/
	void CharacterController::ForwardDirection(const Vector3& forwardDirection)
	{
		forwardDirection_ = forwardDirection;
	}

	/**
	* [JP]
	* 向きたい方向を返す。
	*
	* @brief  向く方向を返す
	* @return 向きたい方向
	*
	* [EN]
	* Returns the direction to face.
	*
	* @brief  Returns the facing direction
	* @return The direction to face
	*/
	const Vector3& CharacterController::ForwardDirection()const
	{
		return forwardDirection_;
	}

	/**
	* [JP]
	* ジャンプする。
	*
	* @brief ジャンプする
	*
	* [EN]
	* Jumps.
	*
	* @brief Jumps
	*/
	void CharacterController::Jump()
	{
		if (!character_)
		{
			return;
		}

		/// [JP] 横の速さはそのままに、上向きの速さをジャンプの強さにする
		/// [EN] Keep the sideways speed and set the upward speed to the jump strength
		JPH::Vec3 velocity = static_cast<JPH::CharacterVirtual*>(character_)->GetLinearVelocity();
		static_cast<JPH::CharacterVirtual*>(character_)->SetLinearVelocity(JPH::Vec3(velocity.GetX(), inheritedVelocity_.y + jumpPower_, velocity.GetZ()));
	}

	/**
	* [JP]
	* 決めた位置へすぐに移動する。
	*
	* @brief 瞬間移動する
	* @param position 移動先（親から見た位置）
	*
	* [EN]
	* Moves straight to a position at once.
	*
	* @brief Teleports
	* @param position Where to go, relative to the parent
	*/
	void CharacterController::Teleport(const Vector3& position)
	{
		if (!character_)
		{
			return;
		}

		/// [JP] 親から見た位置を世界での位置に直して移動する
		/// [EN] Turn the parent-relative position into a world one and move there
		Actor actor = GetActor();
		World& world = actor.GetWorld();
		Entity entity = ActorAccessor::GetEntity(actor);

		Actor parentActor = actor.Parent();
		Vector3 worldPosition = parentActor ? Vector3::TransformPosition(position, parentActor.WorldMatrix()) : position;
		static_cast<JPH::CharacterVirtual*>(character_)->SetPosition(JPH::RVec3(worldPosition.x, worldPosition.y, worldPosition.z));

		/// [JP] 足場の記録を消して、周りの情報を新しくする
		/// [EN] Forget the foothold and refresh what is around it
		foothold_.bodyID_ = ScInvalidBodyID;
		inheritedVelocity_ = Vector3(0.0f, 0.0f, 0.0f);

		actor.GetService<Physics>()->RefreshCharacter(character_);

		Position* positionComponent = world.GetComponent<Position>(entity);
		if (positionComponent)
		{
			Transform::Vector(*positionComponent, position);
		}
	}

	/**
	* [JP]
	* 地面に立っているかを返す。
	*
	* @brief  地面にいるか調べる
	* @return 地面にいれば true
	*
	* [EN]
	* Returns whether it is standing on the ground.
	*
	* @brief  Checks if on the ground
	* @return True if on the ground
	*/
	Bool CharacterController::OnGround()const
	{
		if (!character_)
		{
			return false;
		}

		return static_cast<JPH::CharacterVirtual*>(character_)->GetGroundState() == JPH::CharacterBase::EGroundState::OnGround;
	}

	/**
	* [JP]
	* 壁に触れているかを返す。
	*
	* @brief  壁に触れているか調べる
	* @return 触れていれば true
	*
	* [EN]
	* Returns whether it is touching a wall.
	*
	* @brief  Checks if touching a wall
	* @return True if touching a wall
	*/
	Bool CharacterController::OnWall()const
	{
		if (!character_)
		{
			return false;
		}

		/// [JP] 横向きの面に触れていれば壁
		/// [EN] A sideways surface it touches counts as a wall
		for (const JPH::CharacterContact& contact : static_cast<JPH::CharacterVirtual*>(character_)->GetActiveContacts())
		{
			if (contact.mHadCollision && Abs(contact.mContactNormal.Dot(JPH::Vec3::sAxisY())) < wallNormalDotLimit_)
			{
				return true;
			}
		}

		return false;
	}

	/**
	* [JP]
	* 天井に触れているかを返す。
	*
	* @brief  天井に触れているか調べる
	* @return 触れていれば true
	*
	* [EN]
	* Returns whether it is touching a ceiling.
	*
	* @brief  Checks if touching a ceiling
	* @return True if touching a ceiling
	*/
	Bool CharacterController::OnCeiling()const
	{
		if (!character_)
		{
			return false;
		}

		/// [JP] 下向きの面に触れていれば天井
		/// [EN] A downward surface it touches counts as a ceiling
		for (const JPH::CharacterContact& contact : static_cast<JPH::CharacterVirtual*>(character_)->GetActiveContacts())
		{
			if (contact.mHadCollision && contact.mContactNormal.Dot(JPH::Vec3::sAxisY()) < -wallNormalDotLimit_)
			{
				return true;
			}
		}

		return false;
	}

	/**
	* [JP]
	* 坂の上に立っているかを返す。
	*
	* @brief  坂にいるか調べる
	* @return 坂の上なら true
	*
	* [EN]
	* Returns whether it is standing on a slope.
	*
	* @brief  Checks if on a slope
	* @return True if on a slope
	*/
	Bool CharacterController::OnSlope()const
	{
		if (!character_ || static_cast<JPH::CharacterVirtual*>(character_)->GetGroundState() != JPH::CharacterBase::EGroundState::OnGround)
		{
			return false;
		}

		return static_cast<JPH::CharacterVirtual*>(character_)->GetGroundNormal().Dot(JPH::Vec3::sAxisY()) < 0.999f;
	}

	/**
	* [JP]
	* 立っている地面の向きを返す。
	*
	* @brief  地面の向きを返す
	* @return 地面の向き。なければ真上
	*
	* [EN]
	* Returns which way the ground underneath faces.
	*
	* @brief  Returns the ground normal
	* @return The ground facing, or straight up if there is none
	*/
	Vector3 CharacterController::GroundNormal()const
	{
		if (!character_)
		{
			return Vector3(0.0f, 1.0f, 0.0f);
		}

		JPH::Vec3 normal = static_cast<JPH::CharacterVirtual*>(character_)->GetGroundNormal();
		return Vector3(normal.GetX(), normal.GetY(), normal.GetZ());
	}

	/**
	* [JP]
	* 触れている壁の向きを返す。
	*
	* @brief  壁の向きを返す
	* @return 壁の向き。なければ 0
	*
	* [EN]
	* Returns which way the touched wall faces.
	*
	* @brief  Returns the wall normal
	* @return The wall facing, or zero if there is none
	*/
	Vector3 CharacterController::WallNormal()const
	{
		if (character_)
		{
			for (const JPH::CharacterContact& contact : static_cast<JPH::CharacterVirtual*>(character_)->GetActiveContacts())
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
	* [JP]
	* 触れている天井の向きを返す。
	*
	* @brief  天井の向きを返す
	* @return 天井の向き。なければ 0
	*
	* [EN]
	* Returns which way the touched ceiling faces.
	*
	* @brief  Returns the ceiling normal
	* @return The ceiling facing, or zero if there is none
	*/
	Vector3 CharacterController::CeilingNormal()const
	{
		if (character_)
		{
			for (const JPH::CharacterContact& contact : static_cast<JPH::CharacterVirtual*>(character_)->GetActiveContacts())
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
	* [JP]
	* しゃがんでいるかを返す。
	*
	* @brief  しゃがんでいるか調べる
	* @return しゃがんでいれば true
	*
	* [EN]
	* Returns whether it is crouching.
	*
	* @brief  Checks if crouching
	* @return True if crouching
	*/
	Bool CharacterController::Crouching()const
	{
		return isCrouched_;
	}

	/**
	* [JP]
	* 落ちている最中かを返す。
	*
	* @brief  落ちているか調べる
	* @return 落ちていれば true
	*
	* [EN]
	* Returns whether it is falling.
	*
	* @brief  Checks if falling
	* @return True if falling
	*/
	Bool CharacterController::Falling()const
	{
		if (!character_ || static_cast<JPH::CharacterVirtual*>(character_)->IsSupported())
		{
			return false;
		}

		return static_cast<JPH::CharacterVirtual*>(character_)->GetLinearVelocity().GetY() < 0.0f;
	}

	/**
	* [JP]
	* 空中にいるかを返す。
	*
	* @brief  空中にいるか調べる
	* @return 空中なら true
	*
	* [EN]
	* Returns whether it is in the air.
	*
	* @brief  Checks if in the air
	* @return True if in the air
	*/
	Bool CharacterController::Flying()const
	{
		if (!character_)
		{
			return false;
		}

		return static_cast<JPH::CharacterVirtual*>(character_)->GetGroundState() == JPH::CharacterBase::EGroundState::InAir;
	}

	/**
	* [JP]
	* 何かに支えられて立っているかを返す。
	*
	* @brief  支えられているか調べる
	* @return 支えられていれば true
	*
	* [EN]
	* Returns whether something is holding it up.
	*
	* @brief  Checks if supported
	* @return True if supported
	*/
	Bool CharacterController::Grounded()const
	{
		if (!character_)
		{
			return false;
		}

		return static_cast<JPH::CharacterVirtual*>(character_)->IsSupported();
	}

	/**
	* [JP]
	* 上に飛んでいる最中かを返す。
	*
	* @brief  ジャンプ中か調べる
	* @return ジャンプ中なら true
	*
	* [EN]
	* Returns whether it is moving up through the air.
	*
	* @brief  Checks if jumping
	* @return True if jumping
	*/
	Bool CharacterController::Jumping()const
	{
		if (!character_ || static_cast<JPH::CharacterVirtual*>(character_)->IsSupported())
		{
			return false;
		}

		return static_cast<JPH::CharacterVirtual*>(character_)->GetLinearVelocity().GetY() > 0.0f;
	}

	/**
	* [JP]
	* 地面の上で動いているかを返す。
	*
	* @brief  走っているか調べる
	* @return 動いていれば true
	*
	* [EN]
	* Returns whether it is moving on the ground.
	*
	* @brief  Checks if running
	* @return True if running
	*/
	Bool CharacterController::Running()const
	{
		if (!character_ || !static_cast<JPH::CharacterVirtual*>(character_)->IsSupported())
		{
			return false;
		}

		/// [JP] 横の速さが少しでもあれば動いている
		/// [EN] Any sideways speed counts as moving
		JPH::Vec3 velocity = static_cast<JPH::CharacterVirtual*>(character_)->GetLinearVelocity();
		Float horizontalSpeedSquared = velocity.GetX() * velocity.GetX() + velocity.GetZ() * velocity.GetZ();
		return horizontalSpeedSquared > 0.0001f;
	}

	/**
	* [JP]
	* 地面の上で止まっているかを返す。
	*
	* @brief  止まっているか調べる
	* @return 止まっていれば true
	*
	* [EN]
	* Returns whether it is standing still on the ground.
	*
	* @brief  Checks if stopped
	* @return True if stopped
	*/
	Bool CharacterController::Stopped()const
	{
		if (!character_ || !static_cast<JPH::CharacterVirtual*>(character_)->IsSupported())
		{
			return false;
		}

		/// [JP] 横の速さがほぼなければ止まっている
		/// [EN] Almost no sideways speed counts as stopped
		JPH::Vec3 velocity = static_cast<JPH::CharacterVirtual*>(character_)->GetLinearVelocity();
		Float horizontalSpeedSquared = velocity.GetX() * velocity.GetX() + velocity.GetZ() * velocity.GetZ();
		return horizontalSpeedSquared <= 0.0001f;
	}
}
