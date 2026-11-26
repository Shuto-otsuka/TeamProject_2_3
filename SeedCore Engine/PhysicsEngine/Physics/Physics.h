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
	* [JP]
	* キャラクターを作るときの設定。
	*
	* @brief キャラクターの設定
	*
	* [EN]
	* The settings used to make a character.
	*
	* @brief Character settings
	*/
	struct CharacterDesc
	{
		/// [JP] 最初の位置
		/// [EN] Where it starts
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 最初の向き
		/// [EN] Which way it faces at the start
		Quaternion rotation_ = Quaternion::Identity;

		/// [JP] 体の半径
		/// [EN] The body radius
		Float radius_ = 0.3f;

		/// [JP] 体の高さ
		/// [EN] The body height
		Float height_ = 1.8f;

		/// [JP] 登れる坂の一番急な角度
		/// [EN] The steepest slope it can climb
		Float maxSlopeAngle_ = ToRadians(50.0f);

		/// [JP] 重さ
		/// [EN] The mass
		Float mass_ = 70.0f;

		/// [JP] ほかの体を押せる強さ
		/// [EN] How hard it can push other bodies
		Float maxStrength_ = 100.0f;

		/// [JP] ぶつかる相手を決める層
		/// [EN] The layer that decides what it hits
		ScObjectLayer layer_ = 0;

		/// [JP] 持ち主のアクター
		/// [EN] The actor that owns it
		EntityID userData_;
	};

	/**
	* [JP]
	* 体（Rigidbody）を作るときの設定。
	*
	* @brief 体の設定
	*
	* [EN]
	* The settings used to make a body (Rigidbody).
	*
	* @brief Body settings
	*/
	struct RigidbodyDesc
	{
		/// [JP] 当たり判定の形
		/// [EN] The collision shape
		ScShapeHandle shape_;

		/// [JP] 最初の位置
		/// [EN] Where it starts
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 最初の向き
		/// [EN] Which way it faces at the start
		Quaternion rotation_ = Quaternion::Identity;

		/// [JP] 動き方
		/// [EN] How it moves
		ScMotionType motionType_ = ScMotionType::Dynamic;

		/// [JP] あとで動き方を変えられるようにするか
		/// [EN] Whether its way of moving can change later
		Bool allowMotionTypeChange_ = false;

		/// [JP] 速く動いてもすり抜けないようにするか
		/// [EN] Whether to stop it passing through things at high speed
		Bool continuousCollision_ = false;

		/// [JP] ぶつかる相手を決める層
		/// [EN] The layer that decides what it hits
		ScObjectLayer layer_ = 0;

		/// [JP] 重さ
		/// [EN] The mass
		Float mass_ = 1.0f;

		/// [JP] 動きの止まりやすさ
		/// [EN] How quickly its movement slows down
		Float linearDamping_ = 0.05f;

		/// [JP] 回転の止まりやすさ
		/// [EN] How quickly its spinning slows down
		Float angularDamping_ = 0.05f;

		/// [JP] すべりにくさ
		/// [EN] How grippy it is
		Float friction_ = 0.2f;

		/// [JP] はね返りやすさ
		/// [EN] How bouncy it is
		Float restitution_ = 0.0f;

		/// [JP] 重力のかかり方の倍率
		/// [EN] How strongly gravity pulls it
		Float gravityFactor_ = 1.0f;

		/// [JP] 動いたり回ったりしてよい向き
		/// [EN] Which directions it may move and turn in
		ScAllowedDOFs allowedDOFs_ = ScAllowedDOFsAll;

		/// [JP] 持ち主のアクター
		/// [EN] The actor that owns it
		EntityID userData_;

		/// [JP] 重なりを知らせるだけにするか
		/// [EN] Whether it only reports overlaps
		Bool isSensor_ = false;
	};

	/**
	* [JP]
	* ヒンジのつなぎを作るときの設定。
	*
	* @brief ヒンジの設定
	*
	* [EN]
	* The settings used to make a hinge joint.
	*
	* @brief Hinge settings
	*/
	struct HingeJointDesc
	{
		/// [JP] つなぐ点
		/// [EN] The joining point
		Vector3 anchor_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 回る軸
		/// [EN] The axis it turns around
		Vector3 axis_ = { 0.0f, 1.0f, 0.0f };

		/// [JP] 回れる範囲を決めるか
		/// [EN] Whether to limit how far it turns
		Bool useLimits_ = false;

		/// [JP] 回れる一番小さい角度
		/// [EN] The smallest angle it can turn to
		Float minAngle_ = 0.0f;

		/// [JP] 回れる一番大きい角度
		/// [EN] The largest angle it can turn to
		Float maxAngle_ = 0.0f;
	};

	/**
	* [JP]
	* くっつけるつなぎを作るときの設定。今は何もない。
	*
	* @brief くっつけるつなぎの設定
	*
	* [EN]
	* The settings used to make a fixed joint; there are none yet.
	*
	* @brief Fixed joint settings
	*/
	struct FixedJointDesc
	{
		/// No Code
	};

	/**
	* [JP]
	* ばねのつなぎを作るときの設定。
	*
	* @brief ばねの設定
	*
	* [EN]
	* The settings used to make a spring joint.
	*
	* @brief Spring settings
	*/
	struct SpringJointDesc
	{
		/// [JP] つなぐ点
		/// [EN] The joining point
		Vector3 anchor_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 一番近づける距離
		/// [EN] The closest it can get
		Float minDistance_ = 0.0f;

		/// [JP] 一番離れられる距離
		/// [EN] The farthest it can get
		Float maxDistance_ = 0.0f;

		/// [JP] ばねの硬さ
		/// [EN] How stiff the spring is
		Float frequency_ = 2.0f;

		/// [JP] 揺れの止まりやすさ
		/// [EN] How quickly the bouncing settles
		Float damping_ = 0.5f;
	};

	/**
	* [JP]
	* 滑るつなぎを作るときの設定。
	*
	* @brief 滑るつなぎの設定
	*
	* [EN]
	* The settings used to make a slider joint.
	*
	* @brief Slider settings
	*/
	struct SliderJointDesc
	{
		/// [JP] 滑る向き
		/// [EN] The direction it slides
		Vector3 axis_ = { 1.0f, 0.0f, 0.0f };

		/// [JP] 動ける範囲を決めるか
		/// [EN] Whether to limit how far it slides
		Bool useLimits_ = false;

		/// [JP] 一番近づける距離
		/// [EN] The closest it can get
		Float minDistance_ = 0.0f;

		/// [JP] 一番離れられる距離
		/// [EN] The farthest it can get
		Float maxDistance_ = 0.0f;
	};

	/**
	* [JP]
	* 光線を飛ばして当たったところの情報。
	*
	* @brief 光線が当たった情報
	*
	* [EN]
	* What was hit when a ray was cast.
	*
	* @brief A ray hit
	*/
	struct RaycastHit
	{
		/// [JP] 当たった位置
		/// [EN] Where it hit
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 当たった面の向き
		/// [EN] Which way the hit surface faces
		Vector3 normal_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 当たるまでの距離
		/// [EN] How far it went before hitting
		Float distance_ = 0.0f;

		/// [JP] 当たったアクター
		/// [EN] The actor that was hit
		EntityID entityID_;
	};

	/**
	* [JP]
	* 2D で光線を飛ばして当たったところの情報。
	*
	* @brief 2D の光線が当たった情報
	*
	* [EN]
	* What was hit when a 2D ray was cast.
	*
	* @brief A 2D ray hit
	*/
	struct RaycastHit2D
	{
		/// [JP] 当たった位置
		/// [EN] Where it hit
		Vector2 position_ = { 0.0f, 0.0f };

		/// [JP] 当たった面の向き
		/// [EN] Which way the hit surface faces
		Vector2 normal_ = { 0.0f, 0.0f };

		/// [JP] 当たるまでの距離
		/// [EN] How far it went before hitting
		Float distance_ = 0.0f;

		/// [JP] 当たったアクター
		/// [EN] The actor that was hit
		EntityID entityID_;
	};

	/**
	* [JP]
	* やわらかい体（Softbody）を作るときの設定。
	*
	* @brief やわらかい体の設定
	*
	* [EN]
	* The settings used to make a soft body.
	*
	* @brief Soft body settings
	*/
	struct SoftbodyDesc
	{
		/// [JP] 頂点の位置
		/// [EN] The vertex positions
		DynamicArray<Vector3> positions_;

		/// [JP] 三角形の頂点の並び
		/// [EN] The vertex order of the triangles
		DynamicArray<Uint32> indices_;

		/// [JP] 最初の位置
		/// [EN] Where it starts
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 最初の向き
		/// [EN] Which way it faces at the start
		Quaternion rotation_ = Quaternion::Identity;

		/// [JP] ぶつかる相手を決める層
		/// [EN] The layer that decides what it hits
		ScObjectLayer layer_ = 0;

		/// [JP] 辺の伸びやすさ
		/// [EN] How easily the edges stretch
		Float edgeCompliance_ = 0.0f;

		/// [JP] ゆがみやすさ
		/// [EN] How easily it shears
		Float shearCompliance_ = 0.0f;

		/// [JP] 曲がりやすさ
		/// [EN] How easily it bends
		Float bendCompliance_ = 0.0f;

		/// [JP] 形を整える計算のくり返し回数
		/// [EN] How many times the shape is solved each step
		Uint32 numIterations_ = 5;

		/// [JP] 動きの止まりやすさ
		/// [EN] How quickly its movement slows down
		Float linearDamping_ = 0.1f;

		/// [JP] 中から押す力（ふくらみ）
		/// [EN] How hard it pushes out from inside
		Float pressure_ = 0.0f;

		/// [JP] すべりにくさ
		/// [EN] How grippy it is
		Float friction_ = 0.2f;

		/// [JP] はね返りやすさ
		/// [EN] How bouncy it is
		Float restitution_ = 0.0f;

		/// [JP] 重力のかかり方の倍率
		/// [EN] How strongly gravity pulls it
		Float gravityFactor_ = 1.0f;

		/// [JP] 持ち主のアクター
		/// [EN] The actor that owns it
		EntityID userData_;
	};

	/**
	* [JP]
	* 重なりを調べるときの形の種類。
	*
	* @brief 重なりを調べる形
	*
	* [EN]
	* The kind of shape used to check overlaps.
	*
	* @brief Overlap shape kind
	*/
	enum class OverlapShapeKind :Uint32
	{
		/// [JP] 球
		/// [EN] A sphere
		Sphere,

		/// [JP] 箱
		/// [EN] A box
		Box,

		/// [JP] カプセル
		/// [EN] A capsule
		Capsule,

		/// [JP] 円柱
		/// [EN] A cylinder
		Cylinder,

		/// [JP] 円（2D）
		/// [EN] A circle (2D)
		Circle,

		/// [JP] 四角（2D）
		/// [EN] A rectangle (2D)
		Rect,
	};

	/**
	* [JP]
	* 物理の窓口。形・体・つなぎを作ったり、光線を飛ばして調べたりできる。
	* 2D は 1 メートルを 100 ピクセルとして扱う。
	*
	* @brief 物理の窓口
	*
	* [EN]
	* The front door for physics: make shapes, bodies and joints, and cast rays to look around.
	* 2D treats 1 meter as 100 pixels.
	*
	* @brief The physics front door
	*/
	class SEEDCORE_API Physics
	{
	public:
		/**
		* [JP]
		* 使う世界と、スクリプトから呼ぶ窓口を受け取る。
		*
		* @brief 準備する
		* @param world         使う世界
		* @param queryInstance スクリプトから呼ぶ窓口
		*
		* [EN]
		* Takes the world to use and the door scripts call through.
		*
		* @brief Gets ready
		* @param world         The world to use
		* @param queryInstance The door scripts call through
		*/
		Physics(World& world, QueryInstance& queryInstance);

		~Physics() = default;

	public:
		/**
		* [JP]
		* 箱の形を作る。
		*
		* @brief  箱の形を作る
		* @param  size   大きさ
		* @param  center 中心のずれ
		* @return 形の番号
		*
		* [EN]
		* Makes a box shape.
		*
		* @brief  Makes a box shape
		* @param  size   The size
		* @param  center How far the center is moved
		* @return The shape number
		*/
		ScShapeHandle CreateBoxShape(const Vector3& size, const Vector3& center = { 0.0f, 0.0f, 0.0f });

		/**
		* [JP]
		* 球の形を作る。
		*
		* @brief  球の形を作る
		* @param  radius 半径
		* @return 形の番号
		*
		* [EN]
		* Makes a sphere shape.
		*
		* @brief  Makes a sphere shape
		* @param  radius The radius
		* @return The shape number
		*/
		ScShapeHandle CreateSphereShape(Float radius);

		/**
		* [JP]
		* カプセルの形を作る。
		*
		* @brief  カプセルの形を作る
		* @param  height 円柱部分の高さ
		* @param  radius 半径
		* @return 形の番号
		*
		* [EN]
		* Makes a capsule shape.
		*
		* @brief  Makes a capsule shape
		* @param  height The height of the middle part
		* @param  radius The radius
		* @return The shape number
		*/
		ScShapeHandle CreateCapsuleShape(Float height, Float radius);

		/**
		* [JP]
		* 円柱の形を作る。
		*
		* @brief  円柱の形を作る
		* @param  height 高さ
		* @param  radius 半径
		* @return 形の番号
		*
		* [EN]
		* Makes a cylinder shape.
		*
		* @brief  Makes a cylinder shape
		* @param  height The height
		* @param  radius The radius
		* @return The shape number
		*/
		ScShapeHandle CreateCylinderShape(Float height, Float radius);

		/**
		* [JP]
		* 2D の四角の形を作る。大きさはピクセルで渡す。
		*
		* @brief  四角の形を作る
		* @param  size   大きさ（ピクセル）
		* @param  center 中心のずれ（ピクセル）
		* @return 形の番号
		*
		* [EN]
		* Makes a 2D rectangle shape; the size is given in pixels.
		*
		* @brief  Makes a rectangle shape
		* @param  size   The size in pixels
		* @param  center How far the center is moved, in pixels
		* @return The shape number
		*/
		ScShapeHandle CreateRectShape(const Vector2& size, const Vector2& center = { 0.0f, 0.0f });

		/**
		* [JP]
		* 2D の円の形を作る。大きさはピクセルで渡す。
		*
		* @brief  円の形を作る
		* @param  radius 半径（ピクセル）
		* @param  center 中心のずれ（ピクセル）
		* @return 形の番号
		*
		* [EN]
		* Makes a 2D circle shape; the size is given in pixels.
		*
		* @brief  Makes a circle shape
		* @param  radius The radius in pixels
		* @param  center How far the center is moved, in pixels
		* @return The shape number
		*/
		ScShapeHandle CreateCircleShape(Float radius, const Vector2& center = { 0.0f, 0.0f });

		/**
		* [JP]
		* 三角形の集まり（メッシュ）から形を作る。
		*
		* @brief  メッシュの形を作る
		* @param  assetID   メッシュのアセット番号
		* @param  positions 頂点の位置
		* @param  indices   三角形の頂点の並び
		* @param  scale     大きさ
		* @return 形の番号。作れなければ空
		* @note   動く体には使えない
		*
		* [EN]
		* Makes a shape from a set of triangles (a mesh).
		*
		* @brief  Makes a mesh shape
		* @param  assetID   The mesh asset number
		* @param  positions The vertex positions
		* @param  indices   The vertex order of the triangles
		* @param  scale     The scale
		* @return The shape number, or empty if it could not be made
		* @note   Not usable on moving bodies
		*/
		ScShapeHandle CreateMeshShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const DynamicArray<Uint32>& indices, const Vector3& scale);

		/**
		* [JP]
		* 点の集まりを包む、へこみのない形（凸包）を作る。
		*
		* @brief  凸包の形を作る
		* @param  assetID   メッシュのアセット番号
		* @param  positions 包む点の位置
		* @param  scale     大きさ
		* @return 形の番号。作れなければ空
		*
		* [EN]
		* Makes a dent-free shape (a convex hull) that wraps a set of points.
		*
		* @brief  Makes a convex shape
		* @param  assetID   The mesh asset number
		* @param  positions The points to wrap
		* @param  scale     The scale
		* @return The shape number, or empty if it could not be made
		*/
		ScShapeHandle CreateConvexShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const Vector3& scale);

		/**
		* [JP]
		* 形を使い終わったことを伝える。誰も使わなくなったら消える。
		*
		* @brief 形を手放す
		* @param handle 形の番号
		*
		* [EN]
		* Says you are done with a shape; it is removed once nobody uses it.
		*
		* @brief Releases a shape
		* @param handle The shape number
		*/
		void ReleaseShape(ScShapeHandle handle);

	public:
		/**
		* [JP]
		* 坂や段差を歩けるキャラクターを作る。
		*
		* @brief  キャラクターを作る
		* @param  desc キャラクターの設定
		* @return キャラクターの目印
		*
		* [EN]
		* Makes a character that can walk up slopes and steps.
		*
		* @brief  Makes a character
		* @param  desc The character settings
		* @return The character handle
		*/
		ScCharacterHandle CreateCharacter(const CharacterDesc& desc);

		/**
		* [JP]
		* キャラクターを1回分動かす。
		*
		* @brief キャラクターを動かす
		* @param character     動かすキャラクター
		* @param elapsedTime   進める時間
		* @param maxSlopeAngle 登れる坂の一番急な角度
		* @param stepHeight    登れる段差の高さ
		*
		* [EN]
		* Moves the character one step.
		*
		* @brief Moves the character
		* @param character     The character to move
		* @param elapsedTime   How much time to move
		* @param maxSlopeAngle The steepest slope it can climb
		* @param stepHeight    How high a step it can climb
		*/
		void UpdateCharacter(ScCharacterHandle character, Float elapsedTime, Float maxSlopeAngle, Float stepHeight);

		/**
		* [JP]
		* キャラクターが乗っている地面の情報を新しくする。
		*
		* @brief 地面の情報を新しくする
		* @param character 調べるキャラクター
		*
		* [EN]
		* Updates what the character knows about the ground under it.
		*
		* @brief Refreshes the ground info
		* @param character The character to refresh
		*/
		void RefreshCharacter(ScCharacterHandle character);

		/**
		* [JP]
		* キャラクターを消す。
		*
		* @brief キャラクターを消す
		* @param character 消すキャラクター。消したあとは空になる
		*
		* [EN]
		* Removes a character.
		*
		* @brief Removes a character
		* @param character The character to remove; it becomes empty afterwards
		*/
		void DestroyCharacter(ScCharacterHandle& character);

		/**
		* [JP]
		* キャラクターの体の高さと半径を変える。周りにぶつかるなら変えない。
		*
		* @brief  体の大きさを変える
		* @param  character 変えるキャラクター
		* @param  height    新しい高さ
		* @param  radius    新しい半径
		* @return 変えられたら true
		*
		* [EN]
		* Changes the character's body height and radius, unless it would hit something.
		*
		* @brief  Resizes the body
		* @param  character The character to change
		* @param  height    The new height
		* @param  radius    The new radius
		* @return True if it changed
		*/
		Bool CharacterHeight(ScCharacterHandle character, Float height, Float radius);

		/**
		* [JP]
		* 重力の向きと強さを返す。
		*
		* @brief  重力を返す
		* @return 重力
		*
		* [EN]
		* Returns the direction and strength of gravity.
		*
		* @brief  Returns gravity
		* @return Gravity
		*/
		Vector3 Gravity()const;

	public:
		/**
		* [JP]
		* 体（Rigidbody）を作る。
		*
		* @brief  体を作る
		* @param  desc 体の設定
		* @return 体の番号。作れなければ無効な番号
		*
		* [EN]
		* Makes a body (Rigidbody).
		*
		* @brief  Makes a body
		* @param  desc The body settings
		* @return The body number, or the invalid number if it could not be made
		*/
		ScBodyID CreateRigidbody(const RigidbodyDesc& desc);

		/**
		* [JP]
		* やわらかい体（Softbody）を作る。
		*
		* @brief  やわらかい体を作る
		* @param  desc やわらかい体の設定
		* @return 体の番号。作れなければ無効な番号
		*
		* [EN]
		* Makes a soft body.
		*
		* @brief  Makes a soft body
		* @param  desc The soft body settings
		* @return The body number, or the invalid number if it could not be made
		*/
		ScBodyID CreateSoftbody(const SoftbodyDesc& desc);

		/**
		* [JP]
		* 体を世界から一時的に外す。
		*
		* @brief 体を止める
		* @param bodyID 体の番号
		*
		* [EN]
		* Takes a body out of the world for now.
		*
		* @brief Suspends a body
		* @param bodyID The body number
		*/
		void SuspendBody(ScBodyID bodyID);

		/**
		* [JP]
		* 外した体を世界に戻す。
		*
		* @brief 体を戻す
		* @param bodyID 体の番号
		*
		* [EN]
		* Puts a suspended body back into the world.
		*
		* @brief Resumes a body
		* @param bodyID The body number
		*/
		void ResumeBody(ScBodyID bodyID);

		/**
		* [JP]
		* 体を消す。
		*
		* @brief 体を消す
		* @param bodyID 体の番号
		*
		* [EN]
		* Removes a body.
		*
		* @brief Removes a body
		* @param bodyID The body number
		*/
		void DestroyBody(ScBodyID bodyID);

		/**
		* [JP]
		* 体の形を変える。
		*
		* @brief 体の形を変える
		* @param bodyID 体の番号
		* @param shape  新しい形
		*
		* [EN]
		* Changes a body's shape.
		*
		* @brief Changes the shape
		* @param bodyID The body number
		* @param shape  The new shape
		*/
		void BodyShape(ScBodyID bodyID, ScShapeHandle shape);

		/**
		* [JP]
		* 体の位置と向きを返す。
		*
		* @brief  位置と向きを返す
		* @param  bodyID 体の番号
		* @return 位置と向きをまとめた行列
		*
		* [EN]
		* Returns a body's position and facing.
		*
		* @brief  Returns the transform
		* @param  bodyID The body number
		* @return A matrix holding the position and facing
		*/
		Matrix BodyTransform(ScBodyID bodyID)const;

		/**
		* [JP]
		* 体の位置と向きを変える。
		*
		* @brief 位置と向きを変える
		* @param bodyID   体の番号
		* @param position 新しい位置
		* @param rotation 新しい向き
		*
		* [EN]
		* Changes a body's position and facing.
		*
		* @brief Sets the transform
		* @param bodyID   The body number
		* @param position The new position
		* @param rotation The new facing
		*/
		void BodyTransform(ScBodyID bodyID, const Vector3& position, const Quaternion& rotation);

		/**
		* [JP]
		* 体の動く速さを返す。
		*
		* @brief  速さを返す
		* @param  bodyID 体の番号
		* @return 速さ
		*
		* [EN]
		* Returns how fast a body moves.
		*
		* @brief  Returns the velocity
		* @param  bodyID The body number
		* @return The velocity
		*/
		Vector3 BodyLinearVelocity(ScBodyID bodyID)const;

		/**
		* [JP]
		* 体の動く速さを変える。
		*
		* @brief 速さを変える
		* @param bodyID         体の番号
		* @param linearVelocity 新しい速さ
		*
		* [EN]
		* Changes how fast a body moves.
		*
		* @brief Sets the velocity
		* @param bodyID         The body number
		* @param linearVelocity The new velocity
		*/
		void BodyLinearVelocity(ScBodyID bodyID, const Vector3& linearVelocity);

		/**
		* [JP]
		* 体の動き方を変える。
		*
		* @brief 動き方を変える
		* @param bodyID     体の番号
		* @param motionType 新しい動き方
		*
		* [EN]
		* Changes how a body moves.
		*
		* @brief Changes the motion type
		* @param bodyID     The body number
		* @param motionType The new way of moving
		*/
		void BodyMotionType(ScBodyID bodyID, ScMotionType motionType);

		/**
		* [JP]
		* 体の回る速さを返す。
		*
		* @brief  回る速さを返す
		* @param  bodyID 体の番号
		* @return 回る速さ
		*
		* [EN]
		* Returns how fast a body spins.
		*
		* @brief  Returns the spin
		* @param  bodyID The body number
		* @return The spin
		*/
		Vector3 BodyAngularVelocity(ScBodyID bodyID)const;

		/**
		* [JP]
		* 体の回る速さを変える。
		*
		* @brief 回る速さを変える
		* @param bodyID          体の番号
		* @param angularVelocity 新しい回る速さ
		*
		* [EN]
		* Changes how fast a body spins.
		*
		* @brief Sets the spin
		* @param bodyID          The body number
		* @param angularVelocity The new spin
		*/
		void BodyAngularVelocity(ScBodyID bodyID, const Vector3& angularVelocity);

		/**
		* [JP]
		* 体の上の点が、この時間でどれだけ動くかを求める。足場に乗ったものを一緒に動かすのに使う。
		*
		* @brief 点の動きを求める
		* @param bodyID          体の番号
		* @param point           調べる点
		* @param elapsedTime     時間
		* @param outDisplacement 動いた量が入る
		*
		* [EN]
		* Works out how far a point on a body moves in this time; used to carry things standing on it.
		*
		* @brief Works out a point's movement
		* @param bodyID          The body number
		* @param point           The point to check
		* @param elapsedTime     The time
		* @param outDisplacement Gets how far it moved
		*/
		void BodyPointDisplacement(ScBodyID bodyID, const Vector3& point, Float elapsedTime, Vector3& outDisplacement)const;

		/**
		* [JP]
		* 体の真下にある足場を探す。
		*
		* @brief 足場を探す
		* @param bodyID        体の番号
		* @param maxSlopeAngle 足場とみなす坂の一番急な角度
		* @param maxDistance   下を探す距離
		* @param outFootholdID 見つけた足場の番号が入る
		*
		* [EN]
		* Looks for a foothold right under a body.
		*
		* @brief Finds a foothold
		* @param bodyID        The body number
		* @param maxSlopeAngle The steepest slope that counts as ground
		* @param maxDistance   How far down to look
		* @param outFootholdID Gets the foothold's body number
		*/
		void BodyFoothold(ScBodyID bodyID, Float maxSlopeAngle, Float maxDistance, ScBodyID& outFootholdID)const;

		/**
		* [JP]
		* 重なりを知らせるだけの体にするかを切り替える。
		*
		* @brief 重なり専用を切り替える
		* @param bodyID    体の番号
		* @param isTrigger 重なりを知らせるだけにするか
		*
		* [EN]
		* Switches whether a body only reports overlaps.
		*
		* @brief Toggles overlap-only
		* @param bodyID    The body number
		* @param isTrigger Whether it only reports overlaps
		*/
		void BodyTrigger(ScBodyID bodyID, Bool isTrigger);

		/**
		* [JP]
		* 体があるかを調べる。
		*
		* @brief  体があるか調べる
		* @param  bodyID 体の番号
		* @return あれば true
		*
		* [EN]
		* Checks whether a body exists.
		*
		* @brief  Checks if a body exists
		* @param  bodyID The body number
		* @return True if it exists
		*/
		Bool BodyExists(ScBodyID bodyID)const;

		/**
		* [JP]
		* やわらかい体の今の頂点の位置を取り出す。
		*
		* @brief 頂点の位置を取り出す
		* @param bodyID       体の番号
		* @param outPositions 頂点の位置が入る
		*
		* [EN]
		* Gets the current vertex positions of a soft body.
		*
		* @brief Gets the vertices
		* @param bodyID       The body number
		* @param outPositions Gets the vertex positions
		*/
		void BodyVertex(ScBodyID bodyID, DynamicArray<Vector3>& outPositions)const;

		/**
		* [JP]
		* 体の持ち主のアクターを返す。
		*
		* @brief  持ち主を返す
		* @param  bodyID 体の番号
		* @return 持ち主のアクター
		*
		* [EN]
		* Returns the actor that owns a body.
		*
		* @brief  Returns the owner
		* @param  bodyID The body number
		* @return The owning actor
		*/
		EntityID BodyEntityID(ScBodyID bodyID)const;

		/**
		* [JP]
		* 世界にある体の番号を全部返す。
		*
		* @brief  体の一覧を返す
		* @return 体の番号の一覧
		*
		* [EN]
		* Returns every body number in the world.
		*
		* @brief  Returns every body
		* @return The body numbers
		*/
		DynamicArray<ScBodyID> BodyList()const;

	public:
		/**
		* [JP]
		* 体に力をかける。重さに合わせて少しずつ速くなる。
		*
		* @brief 力をかける
		* @param bodyID 体の番号
		* @param force  かける力
		*
		* [EN]
		* Pushes a body with a force; it speeds up gradually, depending on its mass.
		*
		* @brief Adds a force
		* @param bodyID The body number
		* @param force  The force
		*/
		void AddForce(ScBodyID bodyID, const Vector3& force);

		/**
		* [JP]
		* 体を一瞬でぽんと押す。
		*
		* @brief 一瞬押す
		* @param bodyID  体の番号
		* @param impulse 押す強さ
		*
		* [EN]
		* Gives a body a quick shove.
		*
		* @brief Adds an impulse
		* @param bodyID  The body number
		* @param impulse How hard to shove
		*/
		void AddImpulse(ScBodyID bodyID, const Vector3& impulse);

		/**
		* [JP]
		* 体を回そうとする力をかける。
		*
		* @brief 回す力をかける
		* @param bodyID 体の番号
		* @param torque 回す力
		*
		* [EN]
		* Applies a turning force to a body.
		*
		* @brief Adds a torque
		* @param bodyID The body number
		* @param torque The turning force
		*/
		void AddTorque(ScBodyID bodyID, const Vector3& torque);

		/**
		* [JP]
		* 体を一瞬でぽんと回す。
		*
		* @brief 一瞬回す
		* @param bodyID         体の番号
		* @param angularImpulse 回す強さ
		*
		* [EN]
		* Gives a body a quick spin.
		*
		* @brief Adds a spin
		* @param bodyID         The body number
		* @param angularImpulse How hard to spin
		*/
		void AddSpin(ScBodyID bodyID, const Vector3& angularImpulse);

	public:
		/**
		* [JP]
		* プログラムで動かす体を、次の位置と向きまで動かす。
		*
		* @brief 目標まで動かす
		* @param bodyID         体の番号
		* @param targetPosition 目標の位置
		* @param targetRotation 目標の向き
		* @param elapsedTime    たどり着くまでの時間
		*
		* [EN]
		* Moves a code-driven body toward its next position and facing.
		*
		* @brief Moves to a target
		* @param bodyID         The body number
		* @param targetPosition The target position
		* @param targetRotation The target facing
		* @param elapsedTime    How long it has to get there
		*/
		void MoveTarget(ScBodyID bodyID, const Vector3& targetPosition, const Quaternion& targetRotation, Float elapsedTime);

	public:
		/**
		* [JP]
		* ヒンジのつなぎを作る。
		*
		* @brief  ヒンジを作る
		* @param  bodyA 1つめの体
		* @param  bodyB 2つめの体。無効なら世界につなぐ
		* @param  desc  ヒンジの設定
		* @return つなぎの番号
		*
		* [EN]
		* Makes a hinge joint.
		*
		* @brief  Makes a hinge joint
		* @param  bodyA The first body
		* @param  bodyB The second body; invalid joins to the world
		* @param  desc  The hinge settings
		* @return The joint number
		*/
		ScConstraintHandle CreateHingeJoint(ScBodyID bodyA, ScBodyID bodyB, const HingeJointDesc& desc);

		/**
		* [JP]
		* くっつけるつなぎを作る。
		*
		* @brief  くっつけるつなぎを作る
		* @param  bodyA 1つめの体
		* @param  bodyB 2つめの体。無効なら世界につなぐ
		* @param  desc  つなぎの設定
		* @return つなぎの番号
		*
		* [EN]
		* Makes a fixed joint.
		*
		* @brief  Makes a fixed joint
		* @param  bodyA The first body
		* @param  bodyB The second body; invalid joins to the world
		* @param  desc  The joint settings
		* @return The joint number
		*/
		ScConstraintHandle CreateFixedJoint(ScBodyID bodyA, ScBodyID bodyB, const FixedJointDesc& desc);

		/**
		* [JP]
		* ばねのつなぎを作る。
		*
		* @brief  ばねを作る
		* @param  bodyA 1つめの体
		* @param  bodyB 2つめの体。無効なら世界につなぐ
		* @param  desc  ばねの設定
		* @return つなぎの番号
		*
		* [EN]
		* Makes a spring joint.
		*
		* @brief  Makes a spring joint
		* @param  bodyA The first body
		* @param  bodyB The second body; invalid joins to the world
		* @param  desc  The spring settings
		* @return The joint number
		*/
		ScConstraintHandle CreateSpringJoint(ScBodyID bodyA, ScBodyID bodyB, const SpringJointDesc& desc);

		/**
		* [JP]
		* 滑るつなぎを作る。
		*
		* @brief  滑るつなぎを作る
		* @param  bodyA 1つめの体
		* @param  bodyB 2つめの体。無効なら世界につなぐ
		* @param  desc  滑るつなぎの設定
		* @return つなぎの番号
		*
		* [EN]
		* Makes a slider joint.
		*
		* @brief  Makes a slider joint
		* @param  bodyA The first body
		* @param  bodyB The second body; invalid joins to the world
		* @param  desc  The slider settings
		* @return The joint number
		*/
		ScConstraintHandle CreateSliderJoint(ScBodyID bodyA, ScBodyID bodyB, const SliderJointDesc& desc);

		/**
		* [JP]
		* つなぎを消す。
		*
		* @brief つなぎを消す
		* @param handle つなぎの番号
		*
		* [EN]
		* Removes a joint.
		*
		* @brief Removes a joint
		* @param handle The joint number
		*/
		void DestroyJoint(ScConstraintHandle handle);

		/**
		* [JP]
		* つないでいる体が世界にいるかに合わせて、つなぎを効かせたり止めたりする。
		*
		* @brief つなぎの有効・無効を合わせる
		*
		* [EN]
		* Turns each joint on or off to match whether its bodies are in the world.
		*
		* @brief Refreshes the joints
		*/
		void RefleshJoint();

	public:
		/**
		* [JP]
		* まっすぐな光線を飛ばして、最初に当たったものを調べる。
		*
		* @brief  光線を飛ばす
		* @param  origin      飛ばし始める位置
		* @param  direction   飛ばす向き
		* @param  maxDistance 飛ばす距離
		* @param  outHit      当たったものの情報が入る
		* @param  layerMask   当てるレイヤーの組み合わせ
		* @return 当たったら true
		*
		* [EN]
		* Casts a straight ray and finds the first thing it hits.
		*
		* @brief  Casts a ray
		* @param  origin      Where the ray starts
		* @param  direction   Which way it goes
		* @param  maxDistance How far it goes
		* @param  outHit      Gets what was hit
		* @param  layerMask   Which layers it can hit
		* @return True if it hit something
		*/
		Bool Raycast(const Vector3& origin, const Vector3& direction, Float maxDistance, RaycastHit& outHit, Uint32 layerMask = 0xFFFFFFFF)const;

		/**
		* [JP]
		* 球を飛ばして、最初に当たったものを調べる。
		*
		* @brief  球を飛ばす
		* @param  origin      飛ばし始める位置
		* @param  radius      球の半径
		* @param  direction   飛ばす向き
		* @param  maxDistance 飛ばす距離
		* @param  outHit      当たったものの情報が入る
		* @param  layerMask   当てるレイヤーの組み合わせ
		* @return 当たったら true
		*
		* [EN]
		* Sweeps a sphere and finds the first thing it hits.
		*
		* @brief  Casts a sphere
		* @param  origin      Where it starts
		* @param  radius      The sphere radius
		* @param  direction   Which way it goes
		* @param  maxDistance How far it goes
		* @param  outHit      Gets what was hit
		* @param  layerMask   Which layers it can hit
		* @return True if it hit something
		*/
		Bool Spherecast(const Vector3& origin, Float radius, const Vector3& direction, Float maxDistance, RaycastHit& outHit, Uint32 layerMask = 0xFFFFFFFF)const;

		/**
		* [JP]
		* 箱を飛ばして、最初に当たったものを調べる。
		*
		* @brief  箱を飛ばす
		* @param  origin      飛ばし始める位置
		* @param  halfExtents 箱の大きさの半分
		* @param  rotation    箱の向き
		* @param  direction   飛ばす向き
		* @param  maxDistance 飛ばす距離
		* @param  outHit      当たったものの情報が入る
		* @param  layerMask   当てるレイヤーの組み合わせ
		* @return 当たったら true
		*
		* [EN]
		* Sweeps a box and finds the first thing it hits.
		*
		* @brief  Casts a box
		* @param  origin      Where it starts
		* @param  halfExtents Half the box size
		* @param  rotation    The box facing
		* @param  direction   Which way it goes
		* @param  maxDistance How far it goes
		* @param  outHit      Gets what was hit
		* @param  layerMask   Which layers it can hit
		* @return True if it hit something
		*/
		Bool Boxcast(const Vector3& origin, const Vector3& halfExtents, const Quaternion& rotation, const Vector3& direction, Float maxDistance, RaycastHit& outHit, Uint32 layerMask = 0xFFFFFFFF)const;

		/**
		* [JP]
		* 形を置いて、重なっているアクターを全部探す。
		*
		* @brief  重なっているものを探す
		* @param  kind       形の種類
		* @param  dimensions 形の大きさ
		* @param  position   置く位置
		* @param  rotation   置く向き
		* @param  layerMask  探すレイヤーの組み合わせ
		* @return 重なっているアクターの一覧
		*
		* [EN]
		* Places a shape and finds every actor it overlaps.
		*
		* @brief  Finds overlaps
		* @param  kind       The kind of shape
		* @param  dimensions The shape size
		* @param  position   Where to place it
		* @param  rotation   Which way it faces
		* @param  layerMask  Which layers to look at
		* @return Every overlapping actor
		*/
		DynamicArray<EntityID> Overlap(OverlapShapeKind kind, const Vector3& dimensions, const Vector3& position, const Quaternion& rotation, Uint32 layerMask = 0xFFFFFFFF)const;

		/**
		* [JP]
		* 2D でまっすぐな光線を飛ばして、最初に当たったものを調べる。
		*
		* @brief  2D の光線を飛ばす
		* @param  origin      飛ばし始める位置（ピクセル）
		* @param  direction   飛ばす向き
		* @param  maxDistance 飛ばす距離（ピクセル）
		* @param  outHit      当たったものの情報が入る
		* @param  layerMask   当てるレイヤーの組み合わせ
		* @return 当たったら true
		*
		* [EN]
		* Casts a straight 2D ray and finds the first thing it hits.
		*
		* @brief  Casts a 2D ray
		* @param  origin      Where the ray starts, in pixels
		* @param  direction   Which way it goes
		* @param  maxDistance How far it goes, in pixels
		* @param  outHit      Gets what was hit
		* @param  layerMask   Which layers it can hit
		* @return True if it hit something
		*/
		Bool Raycast2D(const Vector2& origin, const Vector2& direction, Float maxDistance, RaycastHit2D& outHit, Uint32 layerMask = 0xFFFFFFFF)const;

		/**
		* [JP]
		* 2D で円を飛ばして、最初に当たったものを調べる。
		*
		* @brief  2D の円を飛ばす
		* @param  origin      飛ばし始める位置（ピクセル）
		* @param  radius      円の半径（ピクセル）
		* @param  direction   飛ばす向き
		* @param  maxDistance 飛ばす距離（ピクセル）
		* @param  outHit      当たったものの情報が入る
		* @param  layerMask   当てるレイヤーの組み合わせ
		* @return 当たったら true
		*
		* [EN]
		* Sweeps a 2D circle and finds the first thing it hits.
		*
		* @brief  Casts a 2D circle
		* @param  origin      Where it starts, in pixels
		* @param  radius      The circle radius, in pixels
		* @param  direction   Which way it goes
		* @param  maxDistance How far it goes, in pixels
		* @param  outHit      Gets what was hit
		* @param  layerMask   Which layers it can hit
		* @return True if it hit something
		*/
		Bool Circlecast2D(const Vector2& origin, Float radius, const Vector2& direction, Float maxDistance, RaycastHit2D& outHit, Uint32 layerMask = 0xFFFFFFFF)const;

		/**
		* [JP]
		* 2D で四角を飛ばして、最初に当たったものを調べる。
		*
		* @brief  2D の四角を飛ばす
		* @param  origin      飛ばし始める位置（ピクセル）
		* @param  halfExtents 四角の大きさの半分（ピクセル）
		* @param  rotation    四角の向き（ラジアン）
		* @param  direction   飛ばす向き
		* @param  maxDistance 飛ばす距離（ピクセル）
		* @param  outHit      当たったものの情報が入る
		* @param  layerMask   当てるレイヤーの組み合わせ
		* @return 当たったら true
		*
		* [EN]
		* Sweeps a 2D rectangle and finds the first thing it hits.
		*
		* @brief  Casts a 2D rectangle
		* @param  origin      Where it starts, in pixels
		* @param  halfExtents Half the rectangle size, in pixels
		* @param  rotation    The rectangle angle, in radians
		* @param  direction   Which way it goes
		* @param  maxDistance How far it goes, in pixels
		* @param  outHit      Gets what was hit
		* @param  layerMask   Which layers it can hit
		* @return True if it hit something
		*/
		Bool Boxcast2D(const Vector2& origin, const Vector2& halfExtents, Float rotation, const Vector2& direction, Float maxDistance, RaycastHit2D& outHit, Uint32 layerMask = 0xFFFFFFFF)const;

		/**
		* [JP]
		* 2D で形を置いて、重なっているアクターを全部探す。
		*
		* @brief  2D で重なっているものを探す
		* @param  kind       形の種類
		* @param  dimensions 形の大きさ（ピクセル）
		* @param  position   置く位置（ピクセル）
		* @param  rotation   置く向き（ラジアン）
		* @param  layerMask  探すレイヤーの組み合わせ
		* @return 重なっているアクターの一覧
		*
		* [EN]
		* Places a 2D shape and finds every actor it overlaps.
		*
		* @brief  Finds 2D overlaps
		* @param  kind       The kind of shape
		* @param  dimensions The shape size, in pixels
		* @param  position   Where to place it, in pixels
		* @param  rotation   The angle, in radians
		* @param  layerMask  Which layers to look at
		* @return Every overlapping actor
		*/
		DynamicArray<EntityID> Overlap2D(OverlapShapeKind kind, const Vector2& dimensions, const Vector2& position, Float rotation, Uint32 layerMask = 0xFFFFFFFF)const;

	private:
		/**
		* [JP]
		* 2つの体をつなぐつなぎを作る本体。
		*
		* @brief  つなぎを作る
		* @param  bodyA    1つめの体
		* @param  bodyB    2つめの体。無効なら世界につなぐ
		* @param  settings つなぎの設定
		* @return つなぎの番号
		*
		* [EN]
		* The shared code that makes a joint between two bodies.
		*
		* @brief  Makes a joint
		* @param  bodyA    The first body
		* @param  bodyB    The second body; invalid joins to the world
		* @param  settings The joint settings
		* @return The joint number
		*/
		ScConstraintHandle CreateConstraint(ScBodyID bodyA, ScBodyID bodyB, const JPH::TwoBodyConstraintSettings& settings);

	private:
		/// [JP] 2D で 1 メートルを何ピクセルとするか
		/// [EN] How many pixels make one meter in 2D
		SC_CONSTEX Float pixelsPerMeter_ = 100.0f;

		/// [JP] 物理エンジンを動かす係
		/// [EN] Runs the physics engine
		JoltManager& joltManager_;

		/// [JP] この物理が属する世界
		/// [EN] The world this physics belongs to
		World& world_;

		/// [JP] スクリプトから呼ぶ窓口
		/// [EN] The door scripts call through
		QueryInstance& queryInstance_;
	};
}
