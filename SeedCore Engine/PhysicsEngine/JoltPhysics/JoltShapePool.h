#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/Utility/Handle.h>

namespace JPH
{
	class Shape;
}

namespace SeedCore
{
	/**
	* [JP]
	* 当たり判定の形をまとめて持つ係。
	* 同じ形は1つだけ作って使い回し、誰も使わなくなったら消す。
	*
	* @brief 当たり判定の形をまとめて持つ係
	*
	* [EN]
	* Holds collision shapes together.
	* The same shape is made only once and shared, and removed when nobody uses it.
	*
	* @brief Holds the collision shapes
	*/
	class JoltShapePool
	{
	public:
		JoltShapePool() = default;

		/**
		* [JP]
		* 持っている形を全部手放す。
		*
		* @brief 形を手放す
		*
		* [EN]
		* Lets go of every shape it holds.
		*
		* @brief Lets go of the shapes
		*/
		~JoltShapePool();

		JoltShapePool(const JoltShapePool&) = delete;
		JoltShapePool& operator=(const JoltShapePool&) = delete;

		/**
		* [JP]
		* 箱の形を作る。同じ形があればそれを使い回す。
		*
		* @brief  箱の形を作る
		* @param  size   大きさ
		* @param  center 中心のずれ
		* @return 形の番号
		*
		* [EN]
		* Makes a box shape, or reuses a matching one.
		*
		* @brief  Makes a box shape
		* @param  size   The size
		* @param  center How far the center is moved
		* @return The shape number
		*/
		ScShapeHandle CreateBoxShape(const Vector3& size, const Vector3& center);

		/**
		* [JP]
		* 球の形を作る。同じ形があればそれを使い回す。
		*
		* @brief  球の形を作る
		* @param  radius 半径
		* @return 形の番号
		*
		* [EN]
		* Makes a sphere shape, or reuses a matching one.
		*
		* @brief  Makes a sphere shape
		* @param  radius The radius
		* @return The shape number
		*/
		ScShapeHandle CreateSphereShape(Float radius);

		/**
		* [JP]
		* カプセルの形を作る。同じ形があればそれを使い回す。
		*
		* @brief  カプセルの形を作る
		* @param  height 円柱部分の高さ
		* @param  radius 半径
		* @return 形の番号
		*
		* [EN]
		* Makes a capsule shape, or reuses a matching one.
		*
		* @brief  Makes a capsule shape
		* @param  height The height of the middle part
		* @param  radius The radius
		* @return The shape number
		*/
		ScShapeHandle CreateCapsuleShape(Float height, Float radius);

		/**
		* [JP]
		* 円柱の形を作る。同じ形があればそれを使い回す。
		*
		* @brief  円柱の形を作る
		* @param  height 高さ
		* @param  radius 半径
		* @return 形の番号
		*
		* [EN]
		* Makes a cylinder shape, or reuses a matching one.
		*
		* @brief  Makes a cylinder shape
		* @param  height The height
		* @param  radius The radius
		* @return The shape number
		*/
		ScShapeHandle CreateCylinderShape(Float height, Float radius);

		/**
		* [JP]
		* 2D の四角の形を作る。薄い箱として作る。
		*
		* @brief  四角の形を作る
		* @param  size   大きさ
		* @param  center 中心のずれ
		* @return 形の番号
		*
		* [EN]
		* Makes a 2D rectangle shape as a thin box.
		*
		* @brief  Makes a rectangle shape
		* @param  size   The size
		* @param  center How far the center is moved
		* @return The shape number
		*/
		ScShapeHandle CreateRectShape(const Vector2& size, const Vector2& center);

		/**
		* [JP]
		* 2D の円の形を作る。球として作る。
		*
		* @brief  円の形を作る
		* @param  radius 半径
		* @param  center 中心のずれ
		* @return 形の番号
		*
		* [EN]
		* Makes a 2D circle shape as a sphere.
		*
		* @brief  Makes a circle shape
		* @param  radius The radius
		* @param  center How far the center is moved
		* @return The shape number
		*/
		ScShapeHandle CreateCircleShape(Float radius, const Vector2& center);

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
		* 番号から形そのものを取り出す。
		*
		* @brief  形を取り出す
		* @param  handle 形の番号
		* @return 形。古い番号なら nullptr
		*
		* [EN]
		* Gets the shape itself from its number.
		*
		* @brief  Gets a shape
		* @param  handle The shape number
		* @return The shape, or nullptr for an old number
		*/
		const JPH::Shape* Get(ScShapeHandle handle)const;

		/**
		* [JP]
		* 形を使う人を1人増やす。
		*
		* @brief 使う人を増やす
		* @param handle 形の番号
		*
		* [EN]
		* Adds one more user of a shape.
		*
		* @brief Adds a user
		* @param handle The shape number
		*/
		void AddRef(ScShapeHandle handle);

		/**
		* [JP]
		* 形を使う人を1人減らす。誰もいなくなったら消す。
		*
		* @brief 使う人を減らす
		* @param handle 形の番号
		*
		* [EN]
		* Removes one user of a shape, and removes the shape when nobody is left.
		*
		* @brief Removes a user
		* @param handle The shape number
		*/
		void Release(ScShapeHandle handle);

		/**
		* [JP]
		* 形を全部手放して空にする。
		*
		* @brief 全部手放す
		*
		* [EN]
		* Lets go of every shape and empties the pool.
		*
		* @brief Clears everything
		*/
		void Clear();

	private:
		/**
		* [JP]
		* 作った形を空いている入れ物にしまって、番号をつける。
		*
		* @brief  形をしまう
		* @param  shape    しまう形
		* @param  cacheKey 同じ形を見つけるための鍵
		* @return つけた番号
		*
		* [EN]
		* Puts a new shape in a free slot and gives it a number.
		*
		* @brief  Stores a shape
		* @param  shape    The shape to store
		* @param  cacheKey The key used to find the same shape again
		* @return Its number
		*/
		ScShapeHandle Register(const JPH::Shape* shape, Uint64 cacheKey);

	private:
		/// [JP] 2D の形を作るときの厚みの半分
		/// [EN] Half the thickness used for 2D shapes
		SC_CONSTEX Float flatShapeHalfThickness_ = 0.5f;

		/**
		* [JP]
		* 形の種類。同じ形を見分ける鍵に混ぜる。
		*
		* @brief 形の種類
		*
		* [EN]
		* The kind of shape, mixed into the key that finds matching shapes.
		*
		* @brief The kind of shape
		*/
		enum class ShapeKind : Uint64
		{
			Box = 1,
			Sphere = 2,
			Capsule = 3,
			Cylinder = 4,
			Rect = 5,
			Circle = 6,
			Mesh = 7,
			Convex = 8,
		};

		/**
		* [JP]
		* 形1つ分の入れ物。
		*
		* @brief 形の入れ物
		*
		* [EN]
		* A box that holds one shape.
		*
		* @brief A shape slot
		*/
		struct Slot
		{
			/// [JP] 入っている形。空なら nullptr
			/// [EN] The shape inside, or nullptr when empty
			const JPH::Shape* shape_ = nullptr;

			/// [JP] 使い回した回数。古い番号を見分けるのに使う
			/// [EN] How many times it was reused, to spot old numbers
			Uint64 generation_ = 0;

			/// [JP] 同じ形を見つけるための鍵
			/// [EN] The key used to find the same shape
			Uint64 cacheKey_ = 0;

			/// [JP] 使っている人の数
			/// [EN] How many users it has
			Uint32 refCount_ = 0;
		};

		/// [JP] 形の入れ物の一覧
		/// [EN] Every shape slot
		DynamicArray<Slot> slots_;

		/// [JP] 空いている入れ物の番号
		/// [EN] The numbers of empty slots
		DynamicArray<Uint64> freeIndices_;

		/// [JP] 鍵から形の番号を引く表。同じ形を使い回すのに使う
		/// [EN] A table from key to shape number, used to share matching shapes
		std::unordered_map<Uint64, ScShapeHandle> cache_;
	};
}
