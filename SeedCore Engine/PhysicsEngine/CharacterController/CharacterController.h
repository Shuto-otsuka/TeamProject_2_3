#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [JP]
	* アクターにつけると、歩く・走る・ジャンプする・坂や段差を登るキャラクターとして動く部品。
	* 動く足場に乗ると一緒に動く。
	*
	* @brief キャラクターとして動かす部品
	*
	* [EN]
	* Put it on an actor to make it a character that walks, runs, jumps and climbs slopes and steps.
	* It rides along on moving footholds.
	*
	* @brief A character movement part
	*/
	class SEEDCORE_API CharacterController :public SeedScript
	{
	public:
		/// [JP] 体の半径
		/// [EN] The body radius
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("半径", 0.001f, 100.0f)
		Float radius_ = 0.3f;

		/// [JP] 体の高さ
		/// [EN] The body height
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("高さ", 0.001f, 100.0f)
		Float height_ = 1.8f;

		/// [JP] 重さ
		/// [EN] The mass
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("質量", 0.001f, 1000.0f)
		Float mass_ = 70.0f;

		/// [JP] ほかの体を押せる強さ
		/// [EN] How hard it can push other bodies
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("押し出し力", 0.0f, 10000.0f)
		Float pushForce_ = 100.0f;

		/// [JP] 一番速く動ける速さ
		/// [EN] The fastest it can move
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("最大移動速度", 0.0f, 100.0f)
		Float maxMoveSpeed_ = 5.0f;

		/// [JP] 速くなっていく早さ
		/// [EN] How quickly it speeds up
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("加速度", 0.0f, 1000.0f)
		Float acceleration_ = 20.0f;

		/// [JP] 遅くなっていく早さ
		/// [EN] How quickly it slows down
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("減速度", 0.0f, 1000.0f)
		Float deceleration_ = 20.0f;

		/// [JP] 向きを変える速さ（度/秒）
		/// [EN] How fast it turns, in degrees per second
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("回転速度", 0.0f, 1000.0f)
		Float turnSpeed_ = 10.0f;

		/// [JP] 空中での横の動きの止まりやすさ
		/// [EN] How quickly sideways movement slows in the air
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("空気抵抗", 0.0f, 10.0f)
		Float airDrag_ = 0.0f;

		/// [JP] 登れる坂の一番急な角度
		/// [EN] The steepest slope it can climb
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("最大斜面角度", 0.0f, 89.0f)
		Float maxSlopeAngle_ = 50.0f;

		/// [JP] 登れる段差の高さ
		/// [EN] How high a step it can climb
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("最大許容段差高", 0.0f, 10.0f)
		Float maxStepHeight_ = 0.4f;

		/// [JP] 重力のかかり方の倍率
		/// [EN] How strongly gravity pulls it
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("重力倍率")
		Float gravityScale_ = 1.0f;

		/// [JP] 落ちるときの一番速い速さ
		/// [EN] The fastest it can fall
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("最大落下速度", 0.0f, 1000.0f)
		Float maxFallSpeed_ = 50.0f;

		/// [JP] ジャンプの強さ
		/// [EN] How strong the jump is
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("ジャンプ力", 0.0f, 1000.0f)
		Float jumpPower_ = 5.0f;

		/// [JP] しゃがむか
		/// [EN] Whether to crouch
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("しゃがみ")
		Bool crouch_ = false;

		/// [JP] しゃがんだときの高さ
		/// [EN] The height while crouching
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("しゃがみ時の高さ", 0.001f, 100.0f)
		Float crouchHeight_ = 1.0f;

	public:
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
		void OnAwake();

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
		void OnFixedTick(Float elapsedTime);

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
		void OnDestroy();

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
		void OnRevision();

	public:
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
		void Dispatch();

	public:
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
		void MoveDirection(const Vector3& moveDirection);

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
		const Vector3& MoveDirection()const;

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
		void ForwardDirection(const Vector3& forwardDirection);

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
		const Vector3& ForwardDirection()const;

	public:
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
		void Jump();

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
		void Teleport(const Vector3& position);

	public:
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
		Bool OnGround()const;

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
		Bool OnWall()const;

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
		Bool OnCeiling()const;

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
		Bool OnSlope()const;

	public:
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
		Vector3 GroundNormal()const;

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
		Vector3 WallNormal()const;

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
		Vector3 CeilingNormal()const;

	public:
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
		Bool Crouching()const;

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
		Bool Falling()const;

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
		Bool Flying()const;

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
		Bool Grounded()const;

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
		Bool Jumping()const;

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
		Bool Running()const;

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
		Bool Stopped()const;

	public:
		/// [JP] 動きを自分で書きたいときに登録する。登録されている間は、いつもの動きをしない
		/// [EN] Set this to write your own movement; while it is set, the normal movement is skipped
		SinglecastDelegate<void(Float)> onCustomMove_;

	private:
		/**
		* [JP]
		* 乗っている足場の記録。
		*
		* @brief 乗っている足場
		*
		* [EN]
		* A record of the foothold it stands on.
		*
		* @brief The foothold
		*/
		struct Foothold
		{
			/// [JP] 足場の体の番号
			/// [EN] The foothold's body number
			ScBodyID bodyID_ = ScInvalidBodyID;

			/// [JP] 足場から見たキャラクターの位置
			/// [EN] Where the character is, seen from the foothold
			Vector3 position_ = { 0.0f, 0.0f, 0.0f };

			/// [JP] 足場の向き
			/// [EN] Which way the foothold faces
			Quaternion rotation_ = Quaternion::Identity;
		};

	private:
		/// [JP] 壁や天井とみなす向きの境目
		/// [EN] The limit for counting a surface as a wall or ceiling
		SC_CONSTEX Float wallNormalDotLimit_ = 0.5f;

		/// [JP] 動きたい向き
		/// [EN] The direction to move in
		Vector3 moveDirection_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 向きたい方向
		/// [EN] The direction to face
		Vector3 forwardDirection_ = { 0.0f, 0.0f, 1.0f };

		/// [JP] 今しゃがんでいるか
		/// [EN] Whether it is crouching right now
		Bool isCrouched_ = false;

		/// [JP] 足場から受け取っている速さ
		/// [EN] The speed taken from the foothold
		Vector3 inheritedVelocity_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 乗っている足場
		/// [EN] The foothold it stands on
		Foothold foothold_;

		/// [JP] 物理エンジンの中のキャラクター
		/// [EN] The character inside the physics engine
		ScCharacterHandle character_ = nullptr;

		/// [JP] ぶつかりを知らせる係
		/// [EN] The part that reports hits
		ScCharacterListenerHandle characterContactListener_ = nullptr;
	};
	REGISTER_COMPONENT(CharacterController, "Physics");
}
