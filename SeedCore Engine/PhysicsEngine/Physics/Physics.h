#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/Utility/Handle.h>
#include <FoundationEngine/Interop/QueryInstance.h>

namespace JPH
{
	class TwoBodyConstraintSettings;
}

namespace SeedCore
{
	class JoltManager;
	class World;

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
		/// [EN] Initial world position.
		/// [JP] 初期のワールド位置。
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Initial world rotation.
		/// [JP] 初期のワールド回転。
		Quaternion rotation_ = Quaternion::Identity;

		/// [EN] Radius of the character's capsule.
		/// [JP] キャラクターのカプセルの半径。
		Float radius_ = 0.3f;

		/// [EN] Full height of the character's capsule.
		/// [JP] キャラクターのカプセルの全高。
		Float height_ = 1.8f;

		/// [EN] Steepest slope, in radians, the character can still walk on.
		/// [JP] キャラクターが歩いて登れる最も急な斜面の角度（ラジアン）。
		Float maxSlopeAngle_ = ToRadians(50.0f);

		/// [EN] Mass in kilograms, used when the character pushes other bodies.
		/// [JP] 質量（kg）。キャラクターが他のボディを押すときに使う。
		Float mass_ = 70.0f;

		/// [EN] Largest force the character can push other bodies with.
		/// [JP] キャラクターが他のボディを押せる最大の力。
		Float maxStrength_ = 100.0f;

		/// [EN] Packed object layer of the character's inner body.
		/// [JP] キャラクターの内側ボディのパック済みオブジェクトレイヤー。
		ScObjectLayer layer_ = 0;

		/// [EN] Entity the character represents, stored on its inner body.
		/// [JP] キャラクターが表すエンティティ。内側ボディに持たせる。
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
		/// [EN] Pooled shape the body is made of.
		/// [JP] ボディを形作る、プールの形状。
		ScShapeHandle shape_;

		/// [EN] Initial world position.
		/// [JP] 初期のワールド位置。
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Initial world rotation.
		/// [JP] 初期のワールド回転。
		Quaternion rotation_ = Quaternion::Identity;

		/// [EN] Whether the body is static, kinematic or dynamic.
		/// [JP] ボディがスタティック・キネマティック・ダイナミックのどれか。
		ScMotionType motionType_ = ScMotionType::Dynamic;

		/// [EN] Whether the motion type may be switched after creation.
		/// [JP] 生成後に運動タイプを切り替えてよいか。
		Bool allowMotionTypeChange_ = false;

		/// [EN] Whether fast motion is swept so the body does not tunnel through thin geometry.
		/// [JP] 速い移動を掃引で調べ、薄いジオメトリをすり抜けないようにするか。
		Bool continuousCollision_ = false;

		/// [EN] Packed object layer (motion type, Actor layer, trigger and 2D bits).
		/// [JP] パック済みオブジェクトレイヤー（運動タイプ、Actor レイヤー、トリガーと 2D のビット）。
		ScObjectLayer layer_ = 0;

		/// [EN] Mass in kilograms.
		/// [JP] 質量（kg）。
		Float mass_ = 1.0f;

		/// [EN] Damping that slows linear velocity over time.
		/// [JP] 並進速度を時間とともに弱める減衰。
		Float linearDamping_ = 0.05f;

		/// [EN] Damping that slows angular velocity over time.
		/// [JP] 角速度を時間とともに弱める減衰。
		Float angularDamping_ = 0.05f;

		/// [EN] Friction coefficient.
		/// [JP] 摩擦係数。
		Float friction_ = 0.2f;

		/// [EN] Restitution: 0 does not bounce, 1 bounces back with full speed.
		/// [JP] 反発係数。0 で跳ねず、1 で速さを保って跳ね返る。
		Float restitution_ = 0.0f;

		/// [EN] Multiplier on world gravity; 0 turns gravity off.
		/// [JP] ワールドの重力に掛ける倍率。0 で重力を切る。
		Float gravityFactor_ = 1.0f;

		/// [EN] Axes the body may move and rotate along.
		/// [JP] ボディが移動・回転してよい軸。
		ScAllowedDOFs allowedDOFs_ = ScAllowedDOFsAll;

		/// [EN] Entity the body represents, stored on the body.
		/// [JP] ボディが表すエンティティ。ボディに持たせる。
		EntityID userData_;

		/// [EN] Whether the body only reports overlaps (a trigger) without physical response.
		/// [JP] 物理応答をせず、重なりだけを知らせる（トリガー）か。
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
		/// [EN] Point the hinge turns around.
		/// [JP] ヒンジが回る中心の点。
		Vector3 anchor_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Axis the hinge turns about.
		/// [JP] ヒンジが回る軸。
		Vector3 axis_ = { 0.0f, 1.0f, 0.0f };

		/// [EN] Whether the angle is held between minAngle_ and maxAngle_.
		/// [JP] 角度を minAngle_ と maxAngle_ の間に収めるか。
		Bool useLimits_ = false;

		/// [EN] Lower angle limit.
		/// [JP] 角度の下限。
		Float minAngle_ = 0.0f;

		/// [EN] Upper angle limit.
		/// [JP] 角度の上限。
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
		/// [EN] Point the spring is attached at.
		/// [JP] ばねを取り付ける点。
		Vector3 anchor_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Shortest distance the spring allows between the bodies.
		/// [JP] ばねが許すボディ間の最短距離。
		Float minDistance_ = 0.0f;

		/// [EN] Longest distance the spring allows between the bodies.
		/// [JP] ばねが許すボディ間の最長距離。
		Float maxDistance_ = 0.0f;

		/// [EN] Oscillation frequency of the spring, in Hz; higher is stiffer.
		/// [JP] ばねの振動数（Hz）。高いほど硬い。
		Float frequency_ = 2.0f;

		/// [EN] Damping ratio: 0 keeps oscillating, 1 settles without overshoot.
		/// [JP] 減衰比。0 で振動し続け、1 で行き過ぎずに収まる。
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
		/// [EN] Axis the bodies slide along.
		/// [JP] ボディが滑る軸。
		Vector3 axis_ = { 1.0f, 0.0f, 0.0f };

		/// [EN] Whether the slide is held between minDistance_ and maxDistance_.
		/// [JP] 移動量を minDistance_ と maxDistance_ の間に収めるか。
		Bool useLimits_ = false;

		/// [EN] Lower translation limit.
		/// [JP] 移動量の下限。
		Float minDistance_ = 0.0f;

		/// [EN] Upper translation limit.
		/// [JP] 移動量の上限。
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
		/// [EN] Point where the query hit, on the hit body's surface.
		/// [JP] クエリが当たった点。当たったボディの表面上。
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Surface normal at the hit point.
		/// [JP] 当たった点での面の法線。
		Vector3 normal_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Distance travelled from the origin to the hit.
		/// [JP] 始点から当たった位置までの距離。
		Float distance_ = 0.0f;

		/// [EN] Entity of the hit body.
		/// [JP] 当たったボディのエンティティ。
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
		/// [EN] Point where the query hit, in canvas pixels with Y down.
		/// [JP] クエリが当たった点。Y 下向きの Canvas ピクセル。
		Vector2 position_ = { 0.0f, 0.0f };

		/// [EN] Surface normal at the hit point, with Y down.
		/// [JP] 当たった点での面の法線。Y 下向き。
		Vector2 normal_ = { 0.0f, 0.0f };

		/// [EN] Distance travelled from the origin to the hit, in pixels.
		/// [JP] 始点から当たった位置までの距離（ピクセル）。
		Float distance_ = 0.0f;

		/// [EN] Entity of the hit body.
		/// [JP] 当たったボディのエンティティ。
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
		/// [EN] Vertex positions of the mesh, in the body's local space.
		/// [JP] メッシュの頂点位置。ボディのローカル空間。
		DynamicArray<Vector3> positions_;

		/// [EN] Triangle indices into positions_, three per face.
		/// [JP] positions_ を指す三角形のインデックス。1面につき3つ。
		DynamicArray<Uint32> indices_;

		/// [EN] Initial world position.
		/// [JP] 初期のワールド位置。
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Initial world rotation.
		/// [JP] 初期のワールド回転。
		Quaternion rotation_ = Quaternion::Identity;

		/// [EN] Packed object layer.
		/// [JP] パック済みオブジェクトレイヤー。
		ScObjectLayer layer_ = 0;

		/// [EN] Softness of each edge against stretching; 0 is rigid.
		/// [JP] 各辺の伸びに対する柔らかさ。0 で硬い。
		Float edgeCompliance_ = 0.0f;

		/// [EN] Softness of each face against shearing; 0 is rigid.
		/// [JP] 各面のせん断に対する柔らかさ。0 で硬い。
		Float shearCompliance_ = 0.0f;

		/// [EN] Softness between neighbouring faces against bending; 0 is rigid.
		/// [JP] 隣り合う面の曲げに対する柔らかさ。0 で硬い。
		Float bendCompliance_ = 0.0f;

		/// [EN] Solver iterations per step; more iterations hold the constraints more firmly.
		/// [JP] ステップごとのソルバの反復回数。多いほど拘束がしっかり保たれる。
		Uint32 numIterations_ = 5;

		/// [EN] Damping that slows the vertices' velocity over time.
		/// [JP] 頂点の速度を時間とともに弱める減衰。
		Float linearDamping_ = 0.1f;

		/// [EN] Pressure inside a closed mesh; positive inflates it.
		/// [JP] 閉じたメッシュの内部圧力。正で膨らむ。
		Float pressure_ = 0.0f;

		/// [EN] Friction coefficient.
		/// [JP] 摩擦係数。
		Float friction_ = 0.2f;

		/// [EN] Restitution: 0 does not bounce, 1 bounces back with full speed.
		/// [JP] 反発係数。0 で跳ねず、1 で速さを保って跳ね返る。
		Float restitution_ = 0.0f;

		/// [EN] Multiplier on world gravity; 0 turns gravity off.
		/// [JP] ワールドの重力に掛ける倍率。0 で重力を切る。
		Float gravityFactor_ = 1.0f;

		/// [EN] Entity the body represents, stored on the body.
		/// [JP] ボディが表すエンティティ。ボディに持たせる。
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
		Physics(World& world, QueryInstance& queryInstance);

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
		ScShapeHandle CreateBoxShape(const Vector3& size, const Vector3& center = { 0.0f, 0.0f, 0.0f });

		/**
		* [EN]
		* Creates or reuses a sphere collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 球衝突形状を生成または再利用する。
		*/
		ScShapeHandle CreateSphereShape(Float radius);

		/**
		* [EN]
		* Creates or reuses a capsule collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* カプセル衝突形状を生成または再利用する。
		*/
		ScShapeHandle CreateCapsuleShape(Float height, Float radius);

		/**
		* [EN]
		* Creates or reuses a cylinder collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 円柱衝突形状を生成または再利用する。
		*/
		ScShapeHandle CreateCylinderShape(Float height, Float radius);

		/**
		* [EN]
		* Creates or reuses a rectangular 2D collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 矩形の2D衝突形状を生成または再利用する。
		*/
		ScShapeHandle CreateRectShape(const Vector2& size, const Vector2& center = { 0.0f, 0.0f });

		/**
		* [EN]
		* Creates or reuses a circular 2D collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 円形の2D衝突形状を生成または再利用する。
		*/
		ScShapeHandle CreateCircleShape(Float radius, const Vector2& center = { 0.0f, 0.0f });

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
		ScShapeHandle CreateMeshShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const DynamicArray<Uint32>& indices, const Vector3& scale);

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
		ScShapeHandle CreateConvexShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const Vector3& scale);

		/**
		* [EN]
		* Releases one reference to a pooled collision shape.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プールされた衝突形状への参照を一つ解放する。
		*/
		void ReleaseShape(ScShapeHandle handle);

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
		ScCharacterHandle CreateCharacter(const CharacterDesc& desc);

		/**
		* [EN]
		* Advances character movement and stair handling for one frame.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1フレーム分のキャラクター移動と段差処理を更新する。
		*/
		void UpdateCharacter(ScCharacterHandle character, Float elapsedTime, Float maxSlopeAngle, Float stepHeight);

		/**
		* [EN]
		* Refreshes the contacts of a virtual character.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 仮想キャラクターの接触情報を更新する。
		*/
		void RefreshCharacter(ScCharacterHandle character);

		/**
		* [EN]
		* Releases a virtual character reference.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 仮想キャラクターへの参照を解放する。
		*/
		void DestroyCharacter(ScCharacterHandle& character);

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
		Bool CharacterHeight(ScCharacterHandle character, Float height, Float radius);

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
		ScBodyID CreateRigidbody(const RigidbodyDesc& desc);

		/**
		* [EN]
		* Creates and activates a soft body from mesh data.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* メッシュデータからソフトボディを生成して有効化する。
		*/
		ScBodyID CreateSoftbody(const SoftbodyDesc& desc);

		/**
		* [EN]
		* Removes a body from simulation without destroying it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディを破棄せずシミュレーションから外す。
		*/
		void SuspendBody(ScBodyID bodyID);

		/**
		* [EN]
		* Adds a suspended body back to simulation.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 停止中のボディをシミュレーションへ戻す。
		*/
		void ResumeBody(ScBodyID bodyID);

		/**
		* [EN]
		* Removes and destroys a physics body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 物理ボディを削除して破棄する。
		*/
		void DestroyBody(ScBodyID bodyID);

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
		void BodyShape(ScBodyID bodyID, ScShapeHandle shape);

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
		Matrix BodyTransform(ScBodyID bodyID)const;

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
		void BodyTransform(ScBodyID bodyID, const Vector3& position, const Quaternion& rotation);

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
		Vector3 BodyLinearVelocity(ScBodyID bodyID)const;

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
		void BodyLinearVelocity(ScBodyID bodyID, const Vector3& linearVelocity);

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
		void BodyMotionType(ScBodyID bodyID, ScMotionType motionType);

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
		Vector3 BodyAngularVelocity(ScBodyID bodyID)const;

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
		void BodyAngularVelocity(ScBodyID bodyID, const Vector3& angularVelocity);

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
		void BodyTrigger(ScBodyID bodyID, Bool isTrigger);

		/**
		* [EN]
		* Reports whether a body is currently added to the simulation.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディが現在シミュレーションに追加されているかを返す。
		*/
		Bool BodyExists(ScBodyID bodyID)const;

		/**
		* [EN]
		* Writes the world positions of all soft-body vertices.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ソフトボディの全頂点のワールド位置を書き出す。
		*/
		void BodyVertex(ScBodyID bodyID, DynamicArray<Vector3>& outPositions)const;

		/**
		* [EN]
		* Returns the entity identifier stored in a body.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディに格納されたエンティティ識別子を返す。
		*/
		EntityID BodyEntityID(ScBodyID bodyID)const;

		/**
		* [EN]
		* Returns the identifiers of all physics bodies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* すべての物理ボディ識別子を返す。
		*/
		DynamicArray<ScBodyID> BodyList()const;

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
		void AddForce(ScBodyID bodyID, const Vector3& force);

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
		void AddImpulse(ScBodyID bodyID, const Vector3& impulse);

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
		void AddTorque(ScBodyID bodyID, const Vector3& torque);

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
		void AddSpin(ScBodyID bodyID, const Vector3& angularImpulse);

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
		void MoveTarget(ScBodyID bodyID, const Vector3& targetPosition, const Quaternion& targetRotation, Float elapsedTime);

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
		ScConstraintHandle CreateHingeJoint(ScBodyID bodyA, ScBodyID bodyB, const HingeJointDesc& desc);

		/**
		* [EN]
		* Creates a fixed constraint between two bodies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2つのボディ間に固定拘束を生成する。
		*/
		ScConstraintHandle CreateFixedJoint(ScBodyID bodyA, ScBodyID bodyB, const FixedJointDesc& desc);

		/**
		* [EN]
		* Creates a spring distance constraint between two bodies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2つのボディ間にばね距離拘束を生成する。
		*/
		ScConstraintHandle CreateSpringJoint(ScBodyID bodyA, ScBodyID bodyB, const SpringJointDesc& desc);

		/**
		* [EN]
		* Creates a slider constraint between two bodies.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2つのボディ間にスライダー拘束を生成する。
		*/
		ScConstraintHandle CreateSliderJoint(ScBodyID bodyA, ScBodyID bodyB, const SliderJointDesc& desc);

		/**
		* [EN]
		* Releases a joint constraint.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ジョイント拘束を解放する。
		*/
		void DestroyJoint(ScConstraintHandle handle);

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
		* Finds the nearest 3D body intersected by a swept box, given its
		* half extents and rotation.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 半分の大きさと回転を指定した、移動する箱と交差する最も近い
		* 3Dボディを検索する。
		*/
		Bool Boxcast(const Vector3& origin, const Vector3& halfExtents, const Quaternion& rotation, const Vector3& direction, Float maxDistance, RaycastHit& outHit, Uint32 layerMask = 0xFFFFFFFF)const;

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
		* Finds the nearest planar body intersected by a swept rectangle,
		* given its half extents (pixels) and rotation (degrees).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 半分の大きさ（ピクセル）と回転（度）を指定した、移動する矩形と
		* 交差する最も近い平面ボディを検索する。
		*/
		Bool Boxcast2D(const Vector2& origin, const Vector2& halfExtents, Float rotation, const Vector2& direction, Float maxDistance, RaycastHit2D& outHit, Uint32 layerMask = 0xFFFFFFFF)const;

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
		ScConstraintHandle CreateConstraint(ScBodyID bodyA, ScBodyID bodyB, const JPH::TwoBodyConstraintSettings& settings);

	private:
		/// [EN] Number of screen pixels represented by one physics meter.
		/// [JP] 物理空間の1メートルに対応する画面上のピクセル数。
		SC_CONSTEX Float pixelsPerMeter_ = 100.0f;

		/// [EN] Jolt services and pooled physics resources used by this facade.
		/// [JP] このファサードが使用するJoltサービスと物理リソースプール。
		JoltManager& joltManager_;

		/// [EN] World that owns this physics facade.
		/// [JP] この物理ファサードを所有する World。
		World& world_;

		/// [EN] Record every query is added to, owned by World.
		/// [JP] クエリをすべて記録する先。World が所有する。
		QueryInstance& queryInstance_;
	};
}
