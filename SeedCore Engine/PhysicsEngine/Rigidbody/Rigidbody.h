#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/SeedScript.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	/**
	* [JP]
	* アクターにつけると、重さや力で動いたり、ぶつかったりするようになる部品。
	* 形は同じアクターのコライダーから取る。コライダーがなければ球になる。
	*
	* @brief 物理で動く体の部品
	*
	* [EN]
	* Put it on an actor to make it move by weight and forces and bump into things.
	* Its shape comes from a collider on the same actor, or a sphere if there is none.
	*
	* @brief A physics body part
	*/
	class SEEDCORE_API Rigidbody :public SeedScript
	{
	private:
		friend class PhysicsSystem;

	public:
		/**
		* [JP]
		* 体の動き方の種類。
		*
		* @brief 体の動き方
		*
		* [EN]
		* How the body moves.
		*
		* @brief How the body moves
		*/
		enum class BodyType
		{
			/// [JP] 重さや力で動く
			/// [EN] Moved by weight and forces
			Dynamic,

			/// [JP] プログラムで動かす。ぶつかっても押されない
			/// [EN] Moved by code; it is not pushed by hits
			Kinematic,

			/// [JP] 動かない
			/// [EN] Never moves
			Static,
		};

	public:
		/// [JP] 体の動き方
		/// [EN] How the body moves
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("動作モード")
		BodyType bodyType_ = BodyType::Dynamic;

		/// [JP] 速く動いてもすり抜けないようにするか
		/// [EN] Whether to stop it passing through things at high speed
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_CONDITION(bodyType_ == BodyType::Dynamic)
		SC_REFLECTION_FIELD_EX("連続衝突判定")
		Bool continuousCollision_ = false;

		/// [JP] 重さ
		/// [EN] The mass
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("質量", 0.001f, 100.0f)
		Float mass_ = 1.0f;

		/// [JP] 動きの止まりやすさ（空気の抵抗）
		/// [EN] How quickly its movement slows down (air drag)
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("空気抵抗", 0.0f, 10.0f)
		Float linearDrag_ = 0.5f;

		/// [JP] 回転の止まりやすさ
		/// [EN] How quickly its spinning slows down
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("角抵抗", 0.0f, 10.0f)
		Float angularDrag_ = 0.5f;

		/// [JP] すべりにくさ
		/// [EN] How grippy it is
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("摩擦係数", 0.0f, 1.0f)
		Float friction_ = 0.2f;

		/// [JP] はね返りやすさ
		/// [EN] How bouncy it is
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("反発係数", 0.0f, 1.0f)
		Float restitution_ = 0.5f;

		/// [JP] 重力を受けるか
		/// [EN] Whether gravity pulls it
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("重力")
		Bool useGravity_ = true;

		/// [JP] 重力のかかり方の倍率
		/// [EN] How strongly gravity pulls it
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_CONDITION(useGravity_)
		SC_REFLECTION_FIELD_EX("重力倍率")
		Float gravityScale_ = 1.0f;

		/// [JP] 動く足場に乗ったとき、一緒に動くか
		/// [EN] Whether it rides along on a moving foothold
		SC_REFLECTION_FIELD_CONDITION(bodyType_ == BodyType::Dynamic && !isTrigger_)
		SC_REFLECTION_FIELD_EX("足場追従")
		Bool followFoothold_ = false;

		/// [JP] 足場とみなす坂の一番急な角度
		/// [EN] The steepest slope that counts as a foothold
		SC_REFLECTION_FIELD_CONDITION(followFoothold_)
		SC_REFLECTION_CLAMPED_EX("足場の最大斜面角度", 0.0f, 89.0f)
		Float footholdSlopeAngle_ = 50.0f;

		/// [JP] X 方向に動かないようにするか
		/// [EN] Whether to lock movement on X
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("位置X軸固定")
		Bool freezePositionX_ = false;

		/// [JP] Y 方向に動かないようにするか
		/// [EN] Whether to lock movement on Y
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("位置Y軸固定")
		Bool freezePositionY_ = false;

		/// [JP] Z 方向に動かないようにするか
		/// [EN] Whether to lock movement on Z
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("位置Z軸固定")
		Bool freezePositionZ_ = false;

		/// [JP] X 軸で回らないようにするか
		/// [EN] Whether to lock turning on X
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("回転X軸固定")
		Bool freezeRotationX_ = false;

		/// [JP] Y 軸で回らないようにするか
		/// [EN] Whether to lock turning on Y
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("回転Y軸固定")
		Bool freezeRotationY_ = false;

		/// [JP] Z 軸で回らないようにするか
		/// [EN] Whether to lock turning on Z
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("回転Z軸固定")
		Bool freezeRotationZ_ = false;

		/// [JP] ぶつからずに、重なったことだけを知らせるか
		/// [EN] Whether it only reports overlaps instead of blocking
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("トリガー")
		Bool isTrigger_ = false;

	public:
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
		void OnAttach();

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
		void OnDetach();

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
		void OnAwake();

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
		void OnFixedTick(Float elapsedTime);

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
		void OnDestroy();

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
		void OnRevision();

	public:
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
		void Dispatch();

	public:
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
		void AddForce(const Vector3& force);

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
		void AddImpulse(const Vector3& impulse);

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
		void AddTorque(const Vector3& torque);

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
		void AddSpin(const Vector3& angularImpulse);

	public:
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
		void MoveTarget(const Vector3& targetPosition, const Quaternion& targetRotation, Float elapsedTime);

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
		void MoveTo(const Vector3& targetPosition, const Quaternion& targetRotation, Float duration);

	public:
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
		ScBodyID BodyID()const;

	private:
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
		void Apply(Float elapsedTime);

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
		void ApplyLinearVelocity();

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
		void ApplyAngularVelocity();

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
		void ApplyMove(Float elapsedTime);

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
		void ApplyFoothold(Float elapsedTime);

	private:
		/// [JP] コライダーがないときに使う球の半径
		/// [EN] The sphere radius used when there is no collider
		SC_CONSTEX Float defaultShapeRadius_ = 0.5f;

		/// [JP] 2D で 1 メートルを何ピクセルとするか
		/// [EN] How many pixels make one meter in 2D
		SC_CONSTEX Float pixelsPerMeter_ = 100.0f;

		/// [JP] 物理の体の番号
		/// [EN] The physics body number
		ScBodyID bodyID_ = ScInvalidBodyID;

		/// [JP] 使っている形
		/// [EN] The shape in use
		ScShapeHandle shapeHandle_;

		/// [JP] 最後に速さの部品と合わせた速さ。変わったかを見るのに使う
		/// [EN] The speed last matched with the velocity part, to spot changes
		Vector3 syncedLinearVelocity_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 最後に回る速さの部品と合わせた回る速さ
		/// [EN] The spin last matched with the spin part
		Vector3 syncedAngularVelocity_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] MoveTo で動いている最中か
		/// [EN] Whether a MoveTo is in progress
		Bool moving_ = false;

		/// [JP] MoveTo の目標の位置
		/// [EN] The MoveTo target position
		Vector3 moveTargetPosition_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] MoveTo の目標の向き
		/// [EN] The MoveTo target facing
		Quaternion moveTargetRotation_ = { 0.0f, 0.0f, 0.0f, 1.0f };

		/// [JP] MoveTo の残り時間
		/// [EN] Time left for the MoveTo
		Float moveRemainingTime_ = 0.0f;

		/// [JP] 足場から受け取っている速さ
		/// [EN] The speed taken from the foothold
		Vector3 inheritedLinearVelocity_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 足場から受け取っている回る速さ
		/// [EN] The spin taken from the foothold
		Vector3 inheritedAngularVelocity_ = { 0.0f, 0.0f, 0.0f };
	};
	REGISTER_COMPONENT(Rigidbody, "Physics");
}
