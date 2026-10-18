#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Interop/QueryInstance.h>

namespace SeedCore
{
	class JoltManager;

	/**
	* [EN]
	* Describes the initial state of a virtual character.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 仮想キャラクターの初期状態を表す。
	*/
	struct CharacterDesc
	{
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };
		Quaternion rotation_ = Quaternion::Identity;
		Float radius_ = 0.3f;
		Float height_ = 1.8f;
		Float maxSlopeAngle_ = ToRadians(50.0f);
		Float mass_ = 70.0f;
		Float maxStrength_ = 100.0f;
		JPH::ObjectLayer layer_ = 0;
		EntityID userData_;
	};

	/**
	* [EN]
	* Describes the shape and physical properties of a rigid body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 剛体の形状と物理特性を表す。
	*/
	struct RigidbodyDesc
	{
		Handle<JPH::Shape> shape_;
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };
		Quaternion rotation_ = Quaternion::Identity;
		JPH::EMotionType motionType_ = JPH::EMotionType::Dynamic;
		Bool allowMotionTypeChange_ = false;
		Bool continuousCollision_ = false;
		JPH::ObjectLayer layer_ = 0;
		Float mass_ = 1.0f;
		Float linearDamping_ = 0.05f;
		Float angularDamping_ = 0.05f;
		Float friction_ = 0.2f;
		Float restitution_ = 0.0f;
		Float gravityFactor_ = 1.0f;
		JPH::EAllowedDOFs allowedDOFs_ = JPH::EAllowedDOFs::All;
		EntityID userData_;
		Bool isSensor_ = false;
	};

	/**
	* [EN]
	* Describes the anchor, axis, and angular limits of a hinge joint.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ヒンジジョイントのアンカー、軸、角度制限を表す。
	*/
	struct HingeJointDesc
	{
		Vector3 anchor_ = { 0.0f, 0.0f, 0.0f };
		Vector3 axis_ = { 0.0f, 1.0f, 0.0f };
		Bool useLimits_ = false;
		Float minAngle_ = 0.0f;
		Float maxAngle_ = 0.0f;
	};

	/**
	* [EN]
	* Describes a fixed joint using automatically detected attachment points.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 接続点を自動検出する固定ジョイントを表す。
	*/
	struct FixedJointDesc
	{
		/// No Code
	};

	/**
	* [EN]
	* Describes the distance range and spring response of a spring joint.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スプリングジョイントの距離範囲とばね応答を表す。
	*/
	struct SpringJointDesc
	{
		Vector3 anchor_ = { 0.0f, 0.0f, 0.0f };
		Float minDistance_ = 0.0f;
		Float maxDistance_ = 0.0f;
		Float frequency_ = 2.0f;
		Float damping_ = 0.5f;
	};

	/**
	* [EN]
	* Describes the axis and translation limits of a slider joint.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スライダージョイントの軸と移動制限を表す。
	*/
	struct SliderJointDesc
	{
		Vector3 axis_ = { 1.0f, 0.0f, 0.0f };
		Bool useLimits_ = false;
		Float minDistance_ = 0.0f;
		Float maxDistance_ = 0.0f;
	};

	/**
	* [EN]
	* Stores the result of a three-dimensional physics query.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 3次元物理クエリの結果を保持する。
	*/
	struct RaycastHit
	{
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };
		Vector3 normal_ = { 0.0f, 0.0f, 0.0f };
		Float distance_ = 0.0f;
		EntityID entityID_;
	};

	/**
	* [EN]
	* Stores the result of a two-dimensional physics query.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 2次元物理クエリの結果を保持する。
	*/
	struct RaycastHit2D
	{
		Vector2 position_ = { 0.0f, 0.0f };
		Vector2 normal_ = { 0.0f, 0.0f };
		Float distance_ = 0.0f;
		EntityID entityID_;
	};

	/**
	* [EN]
	* Describes the mesh and physical properties of a soft body.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ソフトボディのメッシュと物理特性を表す。
	*/
	struct SoftbodyDesc
	{
		DynamicArray<Vector3> positions_;
		DynamicArray<Uint32> indices_;
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };
		Quaternion rotation_ = Quaternion::Identity;
		JPH::ObjectLayer layer_ = 0;
		Float edgeCompliance_ = 0.0f;
		Float shearCompliance_ = 0.0f;
		Float bendCompliance_ = 0.0f;
		Uint32 numIterations_ = 5;
		Float linearDamping_ = 0.1f;
		Float pressure_ = 0.0f;
		Float friction_ = 0.2f;
		Float restitution_ = 0.0f;
		Float gravityFactor_ = 1.0f;
		EntityID userData_;
	};

	/**
	* [EN]
	* Kind of shape an overlap query places to find what stands inside it.
	* Sphere, Box, Capsule and Cylinder are for Physics::Overlap; Circle and
	* Rect are for Physics::Overlap2D. What each component of the query's
	* dimensions means is noted on each kind.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Overlap クエリが置き、その中に何があるかを調べる形の種類。
	* Sphere・Box・Capsule・Cylinder は Physics::Overlap 用、Circle と Rect
	* は Physics::Overlap2D 用。クエリの dimensions の各成分の意味は、
	* それぞれの種類に記す。
	*/
	enum class OverlapShapeKind :Uint32
	{
		/// [EN] 3D sphere; dimensions.x is the radius.
		/// [JP] 3D の球。dimensions.x が半径。
		Sphere,

		/// [EN] 3D box; dimensions holds the half extents on each axis.
		/// [JP] 3D の箱。dimensions が各軸の半分の大きさ。
		Box,

		/// [EN] 3D capsule standing along Y; dimensions.x is the radius, dimensions.y the half height of its cylinder part.
		/// [JP] Y 軸に沿って立つ 3D のカプセル。dimensions.x が半径、dimensions.y が円柱部分の高さの半分。
		Capsule,

		/// [EN] 3D cylinder standing along Y; dimensions.x is the radius, dimensions.y the half height.
		/// [JP] Y 軸に沿って立つ 3D の円柱。dimensions.x が半径、dimensions.y が高さの半分。
		Cylinder,

		/// [EN] Canvas circle; dimensions.x is the radius in pixels.
		/// [JP] Canvas 上の円。dimensions.x がピクセル単位の半径。
		Circle,

		/// [EN] Canvas rectangle; dimensions holds the half width and half height in pixels.
		/// [JP] Canvas 上の矩形。dimensions がピクセル単位の幅の半分と高さの半分。
		Rect,
	};

	/**
	* [EN]
	* Provides engine-facing creation, control, and query operations for Jolt physics.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt物理に対するエンジン向けの生成、制御、クエリ操作を提供する。
	*/
	class SEEDCORE_API Physics
	{
	public:
		/**
		* [EN]
		* Constructs a physics facade connected to the engine Jolt manager,
		* recording every query into queryInstance.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エンジンのJolt管理機構へ接続された物理ファサードを構築する。
		* クエリはすべて queryInstance に記録する。
		*/
		Physics(QueryInstance& queryInstance);

		/**
		* [EN]
		* Destroys the physics facade.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 物理ファサードを破棄する。
		*/
		~Physics() = default;

	public:
		/**
		* [EN]
		* Creates or reuses a box collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボックス衝突形状を生成または再利用する。
		*/
		Handle<JPH::Shape> CreateBoxShape(const Vector3& size, const Vector3& center = { 0.0f, 0.0f, 0.0f });

		/**
		* [EN]
		* Creates or reuses a sphere collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 球衝突形状を生成または再利用する。
		*/
		Handle<JPH::Shape> CreateSphereShape(Float radius);

		/**
		* [EN]
		* Creates or reuses a capsule collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* カプセル衝突形状を生成または再利用する。
		*/
		Handle<JPH::Shape> CreateCapsuleShape(Float height, Float radius);

		/**
		* [EN]
		* Creates or reuses a cylinder collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 円柱衝突形状を生成または再利用する。
		*/
		Handle<JPH::Shape> CreateCylinderShape(Float height, Float radius);

		/**
		* [EN]
		* Creates or reuses a rectangular 2D collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 矩形の2D衝突形状を生成または再利用する。
		*/
		Handle<JPH::Shape> CreateRectShape(const Vector2& size, const Vector2& center = { 0.0f, 0.0f });

		/**
		* [EN]
		* Creates or reuses a circular 2D collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 円形の2D衝突形状を生成または再利用する。
		*/
		Handle<JPH::Shape> CreateCircleShape(Float radius, const Vector2& center = { 0.0f, 0.0f });

		/**
		* [EN]
		* Creates or reuses a triangle mesh collision shape, with its
		* vertices scaled per axis. Each asset and scale pair is a separate
		* shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 頂点を軸ごとに拡縮した三角形メッシュ衝突形状を生成または再利用する。
		* アセットとスケールの組ごとに別の形状になる。
		*/
		Handle<JPH::Shape> CreateMeshShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const DynamicArray<Uint32>& indices, const Vector3& scale);

		/**
		* [EN]
		* Creates or reuses a convex hull collision shape, with its points
		* scaled per axis. Each asset and scale pair is a separate shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 点を軸ごとに拡縮した凸包衝突形状を生成または再利用する。アセットと
		* スケールの組ごとに別の形状になる。
		*/
		Handle<JPH::Shape> CreateConvexShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const Vector3& scale);

		/**
		* [EN]
		* Releases one reference to a pooled collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プールされた衝突形状への参照を一つ解放する。
		*/
		void ReleaseShape(Handle<JPH::Shape> handle);

	public:
		/**
		* [EN]
		* Creates a virtual character from the supplied description, together
		* with a kinematic inner body that represents it in the physics world.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定した情報から仮想キャラクターを生成し、物理ワールドでその代わりと
		* なるキネマティックな内側ボディも合わせて生成する。
		*/
		JPH::Ref<JPH::CharacterVirtual> CreateCharacter(const CharacterDesc& desc);

		/**
		* [EN]
		* Advances character movement and stair handling for one frame.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1フレーム分のキャラクター移動と段差処理を更新する。
		*/
		void UpdateCharacter(JPH::CharacterVirtual* character, Float elapsedTime, Float maxSlopeAngle, Float stepHeight);

		/**
		* [EN]
		* Refreshes the contacts of a virtual character.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 仮想キャラクターの接触情報を更新する。
		*/
		void RefreshCharacter(JPH::CharacterVirtual* character);

		/**
		* [EN]
		* Releases a virtual character reference.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 仮想キャラクターへの参照を解放する。
		*/
		void DestroyCharacter(JPH::Ref<JPH::CharacterVirtual>& character);

		/**
		* [EN]
		* Replaces the character shape and its inner body's shape using the
		* specified height and radius. Returns false and leaves both unchanged
		* if the new shape does not fit.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定した高さと半径で、キャラクターとその内側ボディの形状を置き換える。
		* 新しい形状が収まらない場合は、どちらも変えずに false を返す。
		*/
		Bool CharacterHeight(JPH::CharacterVirtual* character, Float height, Float radius);

		/**
		* [EN]
		* Returns the gravity vector used by the physics world.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 物理ワールドで使用する重力ベクトルを返す。
		*/
		Vector3 Gravity()const;

	public:
		/**
		* [EN]
		* Creates and activates a rigid body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 剛体を生成して有効化する。
		*/
		JPH::BodyID CreateRigidbody(const RigidbodyDesc& desc);

		/**
		* [EN]
		* Creates and activates a soft body from mesh data.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* メッシュデータからソフトボディを生成して有効化する。
		*/
		JPH::BodyID CreateSoftbody(const SoftbodyDesc& desc);

		/**
		* [EN]
		* Removes a body from simulation without destroying it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディを破棄せずシミュレーションから外す。
		*/
		void SuspendBody(JPH::BodyID bodyID);

		/**
		* [EN]
		* Adds a suspended body back to simulation.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 停止中のボディをシミュレーションへ戻す。
		*/
		void ResumeBody(JPH::BodyID bodyID);

		/**
		* [EN]
		* Removes and destroys a physics body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 物理ボディを削除して破棄する。
		*/
		void DestroyBody(JPH::BodyID bodyID);

		/**
		* [EN]
		* Replaces the collision shape assigned to a body. A dynamic body
		* keeps its mass; only its inertia follows the new shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディに割り当てられた衝突形状を置き換える。動的ボディは質量を保ち、
		* 慣性だけを新しい形状に合わせる。
		*/
		void BodyShape(JPH::BodyID bodyID, Handle<JPH::Shape> shape);

		/**
		* [EN]
		* Returns the world transform of a body: its rotation followed by its
		* translation, with no scale. Read the position with Translation()
		* and the rotation with Quaternion::CreateFromRotationMatrix().
		* Returns the identity for an invalid ID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディのワールド変換を返す。回転の後に平行移動を行う行列で、スケールは
		* 含まない。位置は Translation()、回転は
		* Quaternion::CreateFromRotationMatrix() で取り出す。無効な ID では
		* 単位行列を返す。
		*/
		Matrix BodyTransform(JPH::BodyID bodyID)const;

		/**
		* [EN]
		* Moves a body to a world position and rotation directly, without
		* simulating the motion in between, and without waking it. Does
		* nothing when the body is already there.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディを、間の動きをシミュレーションせずにワールドの位置と回転へ直接
		* 移す。ボディは起こさない。既にその位置と回転にあれば何もしない。
		*/
		void BodyTransform(JPH::BodyID bodyID, const Vector3& position, const Quaternion& rotation);

		/**
		* [EN]
		* Returns the linear velocity (m/s) of a body; zero for a static
		* body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディの速度(m/s)を返す。静的ボディでは 0。
		*/
		Vector3 BodyLinearVelocity(JPH::BodyID bodyID)const;

		/**
		* [EN]
		* Replaces the linear velocity (m/s) of a dynamic or kinematic body
		* and wakes it when the velocity is not zero. A kinematic body keeps
		* the velocity until it is replaced. Has no effect on a static body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 動的またはキネマティックのボディの速度(m/s)を置き換え、速度が 0 で
		* なければボディを起こす。キネマティックのボディは、置き換えられるまで
		* その速度を保つ。静的ボディには効かない。
		*/
		void BodyLinearVelocity(JPH::BodyID bodyID, const Vector3& linearVelocity);

		/**
		* [EN]
		* Switches a rigid body between Dynamic, Kinematic and Static: changes
		* its motion type, the motion-type bits of its object layer, whether
		* it may sleep, and for a kinematic trigger the flag that lets it
		* meet static bodies, so it ends up as if it had been created that
		* way. A body becoming kinematic starts at rest. Leaving Static needs
		* a body created with allowMotionTypeChange_; soft bodies are left
		* as they are.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 剛体を Dynamic・Kinematic・Static の間で切り替える。運動タイプ、
		* オブジェクトレイヤーの運動タイプのビット、眠れるかどうか、キネマ
		* ティックのトリガーではスタティックのボディと組になるためのフラグを
		* 変え、最初からその設定で作ったのと同じ状態にする。キネマティックに
		* なるボディは静止した状態から始める。Static から抜けるには
		* allowMotionTypeChange_ を付けて作ったボディが要る。ソフトボディは
		* そのまま。
		*/
		void BodyMotionType(JPH::BodyID bodyID, JPH::EMotionType motionType);

		/**
		* [EN]
		* Returns the world-space angular velocity (rad/s) of a body; zero
		* for a static body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディのワールド空間の角速度(rad/s)を返す。静的ボディでは 0。
		*/
		Vector3 BodyAngularVelocity(JPH::BodyID bodyID)const;

		/**
		* [EN]
		* Replaces the world-space angular velocity (rad/s) of a dynamic or
		* kinematic body and wakes it when the angular velocity is not zero.
		* A kinematic body keeps the angular velocity until it is replaced.
		* Has no effect on a static body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 動的またはキネマティックのボディのワールド空間の角速度(rad/s)を
		* 置き換え、角速度が 0 でなければボディを起こす。キネマティックの
		* ボディは、置き換えられるまでその角速度を保つ。静的ボディには効かない。
		*/
		void BodyAngularVelocity(JPH::BodyID bodyID, const Vector3& angularVelocity);

		/**
		* [EN]
		* Turns a body into a trigger or back into a solid body: switches its
		* sensor flag, the SENSOR bit of its object layer, and for a
		* kinematic body the flag that lets it meet static bodies, so it
		* ends up as if it had been created that way.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディをトリガーにする、または実体のあるボディへ戻す。センサーの
		* フラグ、オブジェクトレイヤーの SENSOR ビット、キネマティックの
		* ボディではスタティックのボディと組になるためのフラグを切り替え、
		* 最初からその設定で作ったのと同じ状態にする。
		*/
		void BodyTrigger(JPH::BodyID bodyID, Bool isTrigger);

		/**
		* [EN]
		* Reports whether a body is currently added to the simulation.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディが現在シミュレーションに追加されているかを返す。
		*/
		Bool BodyExists(JPH::BodyID bodyID)const;

		/**
		* [EN]
		* Writes the world positions of all soft-body vertices.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ソフトボディの全頂点のワールド位置を書き出す。
		*/
		void BodyVertex(JPH::BodyID bodyID, DynamicArray<Vector3>& outPositions)const;

		/**
		* [EN]
		* Returns the entity identifier stored in a body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディに格納されたエンティティ識別子を返す。
		*/
		EntityID BodyEntityID(JPH::BodyID bodyID)const;

		/**
		* [EN]
		* Returns the identifiers of all physics bodies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* すべての物理ボディ識別子を返す。
		*/
		DynamicArray<JPH::BodyID> BodyList()const;

	public:
		/**
		* [EN]
		* Adds a force (N) at the body's center of mass for the next step
		* and wakes the body. On a soft body the force is spread evenly
		* over its vertices. Non-dynamic bodies are left untouched.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 次のステップの間、ボディの重心に力(N)を加え、ボディを起こす。
		* ソフトボディでは力を頂点へ均等に分ける。動的でないボディには
		* 何もしない。
		*/
		void AddForce(JPH::BodyID bodyID, const Vector3& force);

		/**
		* [EN]
		* Adds an impulse (kg·m/s) at the body's center of mass, changing
		* its velocity at once, and wakes the body. On a soft body every
		* movable vertex gets the same change of velocity. Non-dynamic
		* bodies are left untouched.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディの重心に力積(kg·m/s)を加えて速度を一度に変え、ボディを
		* 起こす。ソフトボディでは、動ける頂点すべてに同じ速度変化を与える。
		* 動的でないボディには何もしない。
		*/
		void AddImpulse(JPH::BodyID bodyID, const Vector3& impulse);

		/**
		* [EN]
		* Adds a torque (N·m) in world space for the next step and wakes
		* the body. Has no effect on a soft body or a non-dynamic body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 次のステップの間、ワールド空間のトルク(N·m)を加え、ボディを起こす。
		* ソフトボディと動的でないボディには効かない。
		*/
		void AddTorque(JPH::BodyID bodyID, const Vector3& torque);

		/**
		* [EN]
		* Adds an angular impulse (N·m·s) in world space, changing the
		* body's angular velocity at once, and wakes the body. Has no
		* effect on a soft body or a non-dynamic body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ワールド空間の角力積(N·m·s)を加えて角速度を一度に変え、ボディを
		* 起こす。ソフトボディと動的でないボディには効かない。
		*/
		void AddSpin(JPH::BodyID bodyID, const Vector3& angularImpulse);

	public:
		/**
		* [EN]
		* Sets the velocities of a kinematic body so that it reaches the
		* target position and rotation after elapsedTime, and wakes the
		* body. Has no effect on a body that is not kinematic.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* elapsedTime 後に目標の位置と回転へ着くようキネマティックボディの
		* 速度を設定し、ボディを起こす。キネマティックでないボディには効かない。
		*/
		void MoveTarget(JPH::BodyID bodyID, const Vector3& targetPosition, const Quaternion& targetRotation, Float elapsedTime);

	public:
		/**
		* [EN]
		* Creates a hinge constraint between two bodies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2つのボディ間にヒンジ拘束を生成する。
		*/
		Handle<JPH::Constraint> CreateHingeJoint(JPH::BodyID bodyA, JPH::BodyID bodyB, const HingeJointDesc& desc);

		/**
		* [EN]
		* Creates a fixed constraint between two bodies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2つのボディ間に固定拘束を生成する。
		*/
		Handle<JPH::Constraint> CreateFixedJoint(JPH::BodyID bodyA, JPH::BodyID bodyB, const FixedJointDesc& desc);

		/**
		* [EN]
		* Creates a spring distance constraint between two bodies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2つのボディ間にばね距離拘束を生成する。
		*/
		Handle<JPH::Constraint> CreateSpringJoint(JPH::BodyID bodyA, JPH::BodyID bodyB, const SpringJointDesc& desc);

		/**
		* [EN]
		* Creates a slider constraint between two bodies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2つのボディ間にスライダー拘束を生成する。
		*/
		Handle<JPH::Constraint> CreateSliderJoint(JPH::BodyID bodyA, JPH::BodyID bodyB, const SliderJointDesc& desc);

		/**
		* [EN]
		* Releases a joint constraint.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ジョイント拘束を解放する。
		*/
		void DestroyJoint(Handle<JPH::Constraint> handle);

		/**
		* [EN]
		* Enables each joint whose bodies are both in the simulation and
		* disables the rest, so suspended bodies do not drag their joints.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 両方のボディがシミュレーション中のジョイントを有効にし、それ以外を
		* 無効にする。停止中のボディにジョイントが引きずられないようにする。
		*/
		void RefleshJoint();

	public:
		/**
		* [EN]
		* Finds the nearest 3D body intersected by a ray.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* レイと交差する最も近い3Dボディを検索する。
		*/
		Bool Raycast(const Vector3& origin, const Vector3& direction, Float maxDistance, RaycastHit& outHit, Uint32 layerMask = 0xFFFFFFFF)const;

		/**
		* [EN]
		* Finds the nearest 3D body intersected by a swept sphere.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 移動する球と交差する最も近い3Dボディを検索する。
		*/
		Bool Spherecast(const Vector3& origin, Float radius, const Vector3& direction, Float maxDistance, RaycastHit& outHit, Uint32 layerMask = 0xFFFFFFFF)const;

		/**
		* [EN]
		* Returns entities overlapping a 3D query shape of the given kind and
		* size, placed at the given pose. Only Sphere, Box, Capsule and
		* Cylinder are 3D kinds; any other kind overlaps nothing.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定した種類と大きさの 3D クエリ形状を指定の姿勢に置き、重なる
		* エンティティを返す。3D の種類は Sphere・Box・Capsule・Cylinder
		* だけで、それ以外の種類は何とも重ならない。
		*/
		DynamicArray<EntityID> Overlap(OverlapShapeKind kind, const Vector3& dimensions, const Vector3& position, const Quaternion& rotation, Uint32 layerMask = 0xFFFFFFFF)const;

		/**
		* [EN]
		* Finds the nearest planar body intersected by a 2D ray.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2Dレイと交差する最も近い平面ボディを検索する。
		*/
		Bool Raycast2D(const Vector2& origin, const Vector2& direction, Float maxDistance, RaycastHit2D& outHit, Uint32 layerMask = 0xFFFFFFFF)const;

		/**
		* [EN]
		* Finds the nearest planar body intersected by a swept circle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 移動する円と交差する最も近い平面ボディを検索する。
		*/
		Bool Circlecast2D(const Vector2& origin, Float radius, const Vector2& direction, Float maxDistance, RaycastHit2D& outHit, Uint32 layerMask = 0xFFFFFFFF)const;

		/**
		* [EN]
		* Returns entities overlapping a canvas query shape of the given kind
		* and size, placed at the given canvas position (pixels, Y down) and
		* rotation (degrees). Only Circle and Rect are canvas kinds; any other
		* kind overlaps nothing.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 指定した種類と大きさの Canvas クエリ形状を、指定の Canvas 上の位置
		* （ピクセル、Y 下向き）と回転（度）に置き、重なるエンティティを返す。
		* Canvas の種類は Circle と Rect だけで、それ以外の種類は何とも
		* 重ならない。
		*/
		DynamicArray<EntityID> Overlap2D(OverlapShapeKind kind, const Vector2& dimensions, const Vector2& position, Float rotation, Uint32 layerMask = 0xFFFFFFFF)const;

	private:
		/**
		* [EN]
		* Creates and registers a two-body constraint.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2ボディ拘束を生成して登録する。
		*/
		Handle<JPH::Constraint> CreateConstraint(JPH::BodyID bodyA, JPH::BodyID bodyB, const JPH::TwoBodyConstraintSettings& settings);

	private:
		/// [EN] Number of screen pixels represented by one physics meter.
		/// [JP] 物理空間の1メートルに対応する画面上のピクセル数。
		SC_CONST Float pixelsPerMeter_ = 100.0f;

		/// [EN] Jolt services and pooled physics resources used by this facade.
		/// [JP] このファサードが使用するJoltサービスと物理リソースプール。
		JoltManager& joltManager_;

		/// [EN] Record every query is added to, owned by World.
		/// [JP] クエリをすべて記録する先。World が所有する。
		QueryInstance& queryInstance_;
	};
}
