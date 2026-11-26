#include <PhysicsEngine/JoltPhysics/JoltConfig.h>
#include <External/JoltPhysics/Jolt/Jolt.h>
#include <External/JoltPhysics/Jolt/Core/Array.h>
#include <External/JoltPhysics/Jolt/Math/Float3.h>
#include <External/JoltPhysics/Jolt/Geometry/IndexedTriangle.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/Shape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/BoxShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/SphereShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/MeshShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/ConvexShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <PhysicsEngine/JoltPhysics/JoltShapePool.h>
#include <FoundationEngine/Math/Random/Hash.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
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
	JoltShapePool::~JoltShapePool()
	{
		for (Slot& slot : slots_)
		{
			if (slot.shape_)
			{
				slot.shape_->Release();
			}
		}
	}

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
	ScShapeHandle JoltShapePool::CreateBoxShape(const Vector3& size, const Vector3& center)
	{
		/// [JP] 種類と大きさから鍵を作り、同じ形があれば使い回す
		/// [EN] Build a key from the kind and size, and reuse a matching shape
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Box));
		key = HashVector3(key, size);
		key = HashVector3(key, center);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		/// [JP] 箱を作り、中心がずれていれば位置をずらした形で包む
		/// [EN] Make the box, and wrap it in a moved shape if the center is off
		const JPH::Vec3 halfExtent{ Max(size.x * 0.5f, JPH::cDefaultConvexRadius),Max(size.y * 0.5f, JPH::cDefaultConvexRadius),Max(size.z * 0.5f, JPH::cDefaultConvexRadius) };

		JPH::ShapeRefC shape = new JPH::BoxShape(halfExtent);

		if (center != Vector3{ 0.0f, 0.0f, 0.0f })
		{
			shape = new JPH::RotatedTranslatedShape(JPH::Vec3{ center.x, center.y, center.z }, JPH::Quat::sIdentity(), shape);
		}

		return Register(shape.GetPtr(), key);
	}

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
	ScShapeHandle JoltShapePool::CreateSphereShape(Float radius)
	{
		/// [JP] 種類と大きさから鍵を作り、同じ形があれば使い回す
		/// [EN] Build a key from the kind and size, and reuse a matching shape
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Sphere));
		key = HashFloat(key, radius);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		JPH::ShapeRefC shape = new JPH::SphereShape(radius);

		return Register(shape.GetPtr(), key);
	}

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
	ScShapeHandle JoltShapePool::CreateCapsuleShape(Float height, Float radius)
	{
		/// [JP] 種類と大きさから鍵を作り、同じ形があれば使い回す
		/// [EN] Build a key from the kind and size, and reuse a matching shape
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Capsule));
		key = HashFloat(key, height);
		key = HashFloat(key, radius);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		JPH::ShapeRefC shape = new JPH::CapsuleShape(height * 0.5f, radius);

		return Register(shape.GetPtr(), key);
	}

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
	ScShapeHandle JoltShapePool::CreateCylinderShape(Float height, Float radius)
	{
		/// [JP] 種類と大きさから鍵を作り、同じ形があれば使い回す
		/// [EN] Build a key from the kind and size, and reuse a matching shape
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Cylinder));
		key = HashFloat(key, height);
		key = HashFloat(key, radius);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		JPH::ShapeRefC shape = new JPH::CylinderShape(height * 0.5f, radius);

		return Register(shape.GetPtr(), key);
	}

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
	ScShapeHandle JoltShapePool::CreateRectShape(const Vector2& size, const Vector2& center)
	{
		/// [JP] 種類と大きさから鍵を作り、同じ形があれば使い回す
		/// [EN] Build a key from the kind and size, and reuse a matching shape
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Rect));
		key = HashVector2(key, size);
		key = HashVector2(key, center);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		/// [JP] 薄い箱を作り、中心がずれていれば位置をずらした形で包む
		/// [EN] Make a thin box, and wrap it in a moved shape if the center is off
		const JPH::Vec3 halfExtent{ Max(size.x * 0.5f, JPH::cDefaultConvexRadius),Max(size.y * 0.5f, JPH::cDefaultConvexRadius),flatShapeHalfThickness_ };

		JPH::ShapeRefC shape = new JPH::BoxShape(halfExtent);

		if (center.x != 0.0f || center.y != 0.0f)
		{
			shape = new JPH::RotatedTranslatedShape(JPH::Vec3{ center.x, center.y, 0.0f }, JPH::Quat::sIdentity(), shape);
		}

		return Register(shape.GetPtr(), key);
	}

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
	ScShapeHandle JoltShapePool::CreateCircleShape(Float radius, const Vector2& center)
	{
		/// [JP] 種類と大きさから鍵を作り、同じ形があれば使い回す
		/// [EN] Build a key from the kind and size, and reuse a matching shape
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Circle));
		key = HashFloat(key, radius);
		key = HashVector2(key, center);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		/// [JP] 球を作り、中心がずれていれば位置をずらした形で包む
		/// [EN] Make a sphere, and wrap it in a moved shape if the center is off
		JPH::ShapeRefC shape = new JPH::SphereShape(radius);

		if (center.x != 0.0f || center.y != 0.0f)
		{
			shape = new JPH::RotatedTranslatedShape(JPH::Vec3{ center.x, center.y, 0.0f }, JPH::Quat::sIdentity(), shape);
		}

		return Register(shape.GetPtr(), key);
	}

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
	ScShapeHandle JoltShapePool::CreateMeshShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const DynamicArray<Uint32>& indices, const Vector3& scale)
	{
		/// [JP] メッシュの番号と大きさから鍵を作り、同じ形があれば使い回す
		/// [EN] Build a key from the mesh number and scale, and reuse a matching shape
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Mesh));
		key = HashCombine(key, static_cast<Uint64>(assetID));
		key = HashVector3(key, scale);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		if (positions.empty() || indices.size() < 3)
		{
			return ScShapeHandle::null();
		}

		/// [JP] 頂点に大きさをかける
		/// [EN] Apply the scale to the vertices
		JPH::VertexList vertices;
		vertices.reserve(positions.size());
		std::ranges::transform(positions, std::back_inserter(vertices), [&scale](const Vector3& position) { return JPH::Float3(position.x * scale.x, position.y * scale.y, position.z * scale.z); });

		/// [JP] 裏返しの大きさなら、三角形の向きを逆にする
		/// [EN] With a flipped scale, turn the triangles the other way
		Bool insideOut = scale.x * scale.y * scale.z < 0.0f;

		JPH::IndexedTriangleList triangles;
		triangles.reserve(indices.size() / 3);
		for (Size cornerIndex = 0; cornerIndex + 2 < indices.size(); cornerIndex += 3)
		{
			if (insideOut)
			{
				triangles.push_back(JPH::IndexedTriangle(indices[cornerIndex], indices[cornerIndex + 2], indices[cornerIndex + 1]));
			}
			else
			{
				triangles.push_back(JPH::IndexedTriangle(indices[cornerIndex], indices[cornerIndex + 1], indices[cornerIndex + 2]));
			}
		}

		/// [JP] 頂点と三角形からメッシュの形を作る
		/// [EN] Build the mesh shape from the vertices and triangles
		JPH::MeshShapeSettings settings(std::move(vertices), std::move(triangles));
		settings.SetEmbedded();

		JPH::ShapeSettings::ShapeResult result = settings.Create();
		if (!result.IsValid())
		{
			SC_LOG_ERROR("メッシュ形状の生成に失敗しました: {}", result.GetError().c_str());
			return ScShapeHandle::null();
		}

		JPH::ShapeRefC shape = result.Get();

		return Register(shape.GetPtr(), key);
	}

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
	ScShapeHandle JoltShapePool::CreateConvexShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const Vector3& scale)
	{
		/// [JP] メッシュの番号と大きさから鍵を作り、同じ形があれば使い回す
		/// [EN] Build a key from the mesh number and scale, and reuse a matching shape
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Convex));
		key = HashCombine(key, static_cast<Uint64>(assetID));
		key = HashVector3(key, scale);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		if (positions.size() < 4)
		{
			return ScShapeHandle::null();
		}

		/// [JP] 点に大きさをかけて、それを包む形を作る
		/// [EN] Apply the scale to the points and build a shape that wraps them
		JPH::Array<JPH::Vec3> points;
		points.reserve(positions.size());
		std::ranges::transform(positions, std::back_inserter(points), [&scale](const Vector3& position) { return JPH::Vec3(position.x * scale.x, position.y * scale.y, position.z * scale.z); });

		JPH::ConvexHullShapeSettings settings(points, JPH::cDefaultConvexRadius);
		settings.SetEmbedded();

		JPH::ShapeSettings::ShapeResult result = settings.Create();
		if (!result.IsValid())
		{
			SC_LOG_ERROR("凸包形状の生成に失敗しました: {}", result.GetError().c_str());
			return ScShapeHandle::null();
		}

		JPH::ShapeRefC shape = result.Get();

		return Register(shape.GetPtr(), key);
	}

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
	const JPH::Shape* JoltShapePool::Get(ScShapeHandle handle)const
	{
		/// [JP] 古い番号なら何も返さない
		/// [EN] Return nothing for an old number
		if (handle.empty() || handle.index_ >= slots_.size())
		{
			return nullptr;
		}

		const Slot& slot = slots_[handle.index_];
		if (slot.generation_ != handle.generation_)
		{
			return nullptr;
		}

		return slot.shape_;
	}

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
	void JoltShapePool::AddRef(ScShapeHandle handle)
	{
		if (handle.empty() || handle.index_ >= slots_.size())
		{
			return;
		}

		Slot& slot = slots_[handle.index_];
		if (slot.generation_ != handle.generation_)
		{
			return;
		}

		slot.refCount_++;
	}

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
	void JoltShapePool::Release(ScShapeHandle handle)
	{
		if (handle.empty() || handle.index_ >= slots_.size())
		{
			return;
		}

		Slot& slot = slots_[handle.index_];
		if (slot.generation_ != handle.generation_ || slot.refCount_ == 0)
		{
			return;
		}

		/// [JP] まだ使う人がいれば消さない
		/// [EN] Keep it while someone still uses it
		if (--slot.refCount_ > 0)
		{
			return;
		}

		/// [JP] 表から外して手放し、入れ物を空きに戻す
		/// [EN] Take it out of the table, let it go and mark the slot free
		cache_.erase(slot.cacheKey_);
		slot.shape_->Release();
		slot.shape_ = nullptr;
		++slot.generation_;
		freeIndices_.push_back(handle.index_);
	}

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
	void JoltShapePool::Clear()
	{
		for (Slot& slot : slots_)
		{
			if (slot.shape_)
			{
				slot.shape_->Release();
			}
			slot.shape_ = nullptr;
			++slot.generation_;
		}
		slots_.clear();
		freeIndices_.clear();
		cache_.clear();
	}

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
	ScShapeHandle JoltShapePool::Register(const JPH::Shape* shape, Uint64 cacheKey)
	{
		/// [JP] 空いている入れ物か、新しい入れ物を使う
		/// [EN] Use a free slot or a new one
		Uint64 index = 0;
		if (!freeIndices_.empty())
		{
			index = freeIndices_.back();
			freeIndices_.pop_back();
		}
		else
		{
			index = slots_.size();
			slots_.emplace_back();
		}

		/// [JP] 形をしまい、番号を作って表に登録する
		/// [EN] Store the shape, make its number and add it to the table
		Slot& slot = slots_[index];
		shape->AddRef();
		slot.shape_ = shape;
		slot.cacheKey_ = cacheKey;
		slot.refCount_ = 1;

		ScShapeHandle handle{};
		handle.index_ = index;
		handle.generation_ = slot.generation_;
		cache_.emplace(cacheKey, handle);
		return handle;
	};
}
