#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/SeedScript.h>
#include <FoundationEngine/Utility/Handle.h>

namespace SeedCore
{
	/**
	* [EN]
	* Component that gives its actor a simulated body. It borrows the
	* shape of a collider on the same actor (a 0.5 m sphere if none) and,
	* each fixed step, copies the body's pose back to Position/Rotation.
	* The actor's LinearVelocity and AngularVelocity are kept in step with
	* the body's velocity and angular velocity in both directions; this
	* component adds them when it is added and removes them when it is
	* removed, so only actors with a body carry velocities.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Actor にシミュレーションされるボディを持たせるコンポーネント。同じ
	* Actor のコライダーの形状を借り(無ければ半径 0.5 m の球)、固定ステップ
	* ごとにボディの姿勢を Position/Rotation へ書き戻す。Actor の
	* LinearVelocity と AngularVelocity は、ボディの速度・角速度と双方向に
	* 同期する。このコンポーネントは追加されたときにそれらを追加し、外された
	* ときに外すので、速度を持つのはボディを持つ actor だけになる。
	*/
	class SEEDCORE_API Rigidbody :public SeedScript
	{
	private:
		friend class PhysicsSystem;

	public:
		/**
		* [EN]
		* How the body moves.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディの動き方。
		*/
		enum class BodyType
		{
			/// [EN] Moved by forces, gravity and collisions.
			/// [JP] 力・重力・衝突で動く。
			Dynamic,

			/// [EN] Moved only by code or animation; pushes dynamic bodies but is not pushed back.
			/// [JP] コードやアニメーションでだけ動く。動的ボディを押すが、押し返されない。
			Kinematic,

			/// [EN] Never moves.
			/// [JP] 動かない。
			Static,
		};

	public:
		/// [EN] How the body moves.
		/// [JP] ボディの動き方。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("動作モード")
		BodyType bodyType_ = BodyType::Dynamic;

		/// [EN] Sweeps the body along its motion so a fast body does not pass through thin geometry.
		/// [JP] 移動経路に沿ってボディを掃引し、高速なボディが薄い物体をすり抜けないようにする。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_CONDITION(bodyType_ == BodyType::Dynamic)
		SC_REFLECTION_FIELD_EX("連続衝突判定")
		Bool continuousCollision_ = false;

		/// [EN] Mass in kilograms; the inertia is derived from the shape at this mass.
		/// [JP] 質量(kg)。慣性はこの質量に合わせて形状から求める。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("質量", 0.001f, 100.0f)
		Float mass_ = 1.0f;

		/// [EN] Damping that slows the body's linear velocity over time.
		/// [JP] 移動速度を時間とともに弱める減衰。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("空気抵抗", 0.0f, 10.0f)
		Float linearDrag_ = 0.5f;

		/// [EN] Damping that slows the body's angular velocity over time.
		/// [JP] 回転速度を時間とともに弱める減衰。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("角抵抗", 0.0f, 10.0f)
		Float angularDrag_ = 0.5f;

		/// [EN] Friction coefficient: 0 slides freely, 1 grips strongly.
		/// [JP] 摩擦係数。0 で滑り、1 で強く止まる。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("摩擦係数", 0.0f, 1.0f)
		Float friction_ = 0.2f;

		/// [EN] Restitution: 0 does not bounce, 1 bounces back with full speed.
		/// [JP] 反発係数。0 で跳ねず、1 で速さを保って跳ね返る。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_CLAMPED_EX("反発係数", 0.0f, 1.0f)
		Float restitution_ = 0.5f;

		/// [EN] Whether world gravity acts on the body.
		/// [JP] ワールドの重力をボディに働かせるか。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("重力")
		Bool useGravity_ = true;

		/// [EN] Multiplier on world gravity.
		/// [JP] ワールドの重力に掛ける倍率。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_CONDITION(useGravity_)
		SC_REFLECTION_FIELD_EX("重力倍率")
		Float gravityScale_ = 1.0f;

		/// [EN] Locks movement along world X.
		/// [JP] ワールドの X 方向の移動を固定する。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("位置X軸固定")
		Bool freezePositionX_ = false;

		/// [EN] Locks movement along world Y.
		/// [JP] ワールドの Y 方向の移動を固定する。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("位置Y軸固定")
		Bool freezePositionY_ = false;

		/// [EN] Locks movement along world Z; has no effect on a canvas body.
		/// [JP] ワールドの Z 方向の移動を固定する。Canvas のボディには効かない。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("位置Z軸固定")
		Bool freezePositionZ_ = false;

		/// [EN] Locks rotation about X; has no effect on a canvas body.
		/// [JP] X 軸まわりの回転を固定する。Canvas のボディには効かない。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("回転X軸固定")
		Bool freezeRotationX_ = false;

		/// [EN] Locks rotation about Y; has no effect on a canvas body.
		/// [JP] Y 軸まわりの回転を固定する。Canvas のボディには効かない。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("回転Y軸固定")
		Bool freezeRotationY_ = false;

		/// [EN] Locks rotation about Z; on a canvas body it locks the in-plane rotation.
		/// [JP] Z 軸まわりの回転を固定する。Canvas のボディでは平面内の回転を固定する。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("回転Z軸固定")
		Bool freezeRotationZ_ = false;

		/// [EN] Whether the body reports overlaps without physical response.
		/// [JP] 物理応答を行わず、重なりだけを知らせるトリガーにするか。
		SC_REFLECTION_FIELD_REVISION()
		SC_REFLECTION_FIELD_EX("トリガー")
		Bool isTrigger_ = false;

	public:
		/**
		* [EN]
		* Adds LinearVelocity and AngularVelocity to the actor the moment
		* this component is added, so the velocities the body is kept in
		* step with exist before any script's OnAwake or OnStart reads them,
		* in the editor as well as during play. A velocity the actor already
		* has is kept as it is.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このコンポーネントが追加された瞬間に、actor へ LinearVelocity と
		* AngularVelocity を追加する。ボディと同期させる速度が、どの
		* スクリプトの OnAwake・OnStart が読むより前に、プレイ中だけでなく
		* エディタでも存在するようにするため。actor が既に持っている速度は
		* そのまま残す。
		*/
		void OnAttach();

		/**
		* [EN]
		* Removes LinearVelocity and AngularVelocity from the actor the
		* moment this component is removed, undoing OnAttach. Destroying the
		* whole actor does not come through here; the velocities go with it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このコンポーネントが外される瞬間に、actor から LinearVelocity と
		* AngularVelocity を外し、OnAttach を元に戻す。actor ごと破棄する
		* ときはここを通らず、速度は actor と一緒に消える。
		*/
		void OnDetach();

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
		void OnAwake();

		/**
		* [EN]
		* Copies the simulated pose of the body back to the actor's
		* Position and Rotation, its linear velocity back to LinearVelocity,
		* and its angular velocity back to AngularVelocity.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シミュレーション後のボディの姿勢を Actor の Position と Rotation へ、
		* 速度を LinearVelocity へ、角速度を AngularVelocity へ書き戻す。
		*/
		void OnFixedTick(Float elapsedTime);

		/**
		* [EN]
		* Destroys the body and releases the reference to its shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディを破棄し、形状への参照を解放する。
		*/
		void OnDestroy();

		void OnRevision();

	public:
		void Dispatch();

	public:
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
		void AddForce(const Vector3& force);

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
		void AddImpulse(const Vector3& impulse);

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
		void AddTorque(const Vector3& torque);

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
		void AddSpin(const Vector3& angularImpulse);

	public:
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
		void MoveTarget(const Vector3& targetPosition, const Quaternion& targetRotation, Float elapsedTime);

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
		void MoveTo(const Vector3& targetPosition, const Quaternion& targetRotation, Float duration);

	public:
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
		ScBodyID BodyID()const;

	private:
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
		* 前のステップからコードが変えたもの(bodyType_ と isTrigger_ と、Actor の
		* LinearVelocity と AngularVelocity)をボディへ渡して次のステップで効く
		* ようにし、MoveTo で始めた移動を elapsedTime の1ステップ分進める。
		* 固定ステップの前に PhysicsSystem から毎回呼ばれる。
		*/
		void Apply(Float elapsedTime);

		/**
		* [EN]
		* Hands the actor's LinearVelocity to the body when it differs from
		* the value last copied back. On a canvas body the velocity is in
		* pixels per second with Y down, and its Z is ignored. A Static body
		* is not affected.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Actor の LinearVelocity が前回書き戻した値と違えばボディへ渡す。
		* Canvas のボディでは速度をピクセル毎秒・Y 下向きで与え、Z は無視する。
		* Static のボディには効かない。
		*/
		void ApplyLinearVelocity();

		/**
		* [EN]
		* Hands the actor's AngularVelocity to the body when it differs from
		* the value last copied back. On a canvas body only Z is used, as the
		* in-plane angular velocity in the same direction as the canvas
		* angle. A Static body is not affected.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Actor の AngularVelocity が前回書き戻した値と違えばボディへ渡す。
		* Canvas のボディでは Z だけを、Canvas の角度と同じ向きの平面内の角速度
		* として使う。Static のボディには効かない。
		*/
		void ApplyAngularVelocity();

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
		void ApplyMove(Float elapsedTime);

	private:
		/// [EN] Radius of the sphere used when the actor has no collider.
		/// [JP] Actor にコライダーが無いときに使う球の半径。
		SC_CONSTEX Float defaultShapeRadius_ = 0.5f;

		/// [EN] Number of canvas pixels in one physics meter.
		/// [JP] 物理の1メートルに当たる Canvas のピクセル数。
		SC_CONSTEX Float pixelsPerMeter_ = 100.0f;

		/// [EN] Jolt ID of the body.
		/// [JP] ボディの Jolt ID。
		ScBodyID bodyID_ = ScInvalidBodyID;

		/// [EN] This component's own reference to the pooled shape.
		/// [JP] プールの形状に対する、このコンポーネント自身の参照。
		ScShapeHandle shapeHandle_;

		/// [EN] Velocity last exchanged with the body, in LinearVelocity's units; a LinearVelocity that differs from it was written by code.
		/// [JP] 最後にボディとやり取りした速度(LinearVelocity の単位)。これと違う LinearVelocity は、コードから書かれたもの。
		Vector3 syncedLinearVelocity_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Angular velocity last exchanged with the body, in AngularVelocity's units; an AngularVelocity that differs from it was written by code.
		/// [JP] 最後にボディとやり取りした角速度(AngularVelocity の単位)。これと違う AngularVelocity は、コードから書かれたもの。
		Vector3 syncedAngularVelocity_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Whether a move started by MoveTo is in progress, including the step after arrival that stops the body.
		/// [JP] MoveTo で始めた移動の途中かどうか。着いた後にボディを止めるステップも含む。
		Bool moving_ = false;

		/// [EN] Target position of the move, already in physics space (meters, Y up).
		/// [JP] 移動の目標位置。物理空間(メートル・Y 上向き)へ変換済み。
		Vector3 moveTargetPosition_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Target rotation of the move, already in physics space.
		/// [JP] 移動の目標回転。物理空間へ変換済み。
		Quaternion moveTargetRotation_ = { 0.0f, 0.0f, 0.0f, 1.0f };

		/// [EN] Seconds left until the body reaches the target; zero or less means it arrived in the last step.
		/// [JP] ボディが目標に着くまでの残り秒数。0 以下なら直前のステップで着いている。
		Float moveRemainingTime_ = 0.0f;
	};
	REGISTER_COMPONENT(Rigidbody, "Physics");
}
