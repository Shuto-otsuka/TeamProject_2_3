#include <External/JoltPhysics/Jolt/Jolt.h>
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
	* [EN]
	* Creates or reuses a box shape with the specified size and center.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定したサイズと中心を持つボックス形状を生成または再利用する。
	*/
	ScShapeHandle JoltShapePool::CreateBoxShape(const Vector3& size, const Vector3& center)
	{
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Box));
		key = HashVector3(key, size);
		key = HashVector3(key, center);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		const JPH::Vec3 halfExtent{ Max(size.x * 0.5f, JPH::cDefaultConvexRadius),Max(size.y * 0.5f, JPH::cDefaultConvexRadius),Max(size.z * 0.5f, JPH::cDefaultConvexRadius) };

		JPH::ShapeRefC shape = new JPH::BoxShape(halfExtent);

		if (center != Vector3{ 0.0f, 0.0f, 0.0f })
		{
			shape = new JPH::RotatedTranslatedShape(JPH::Vec3{ center.x, center.y, center.z }, JPH::Quat::sIdentity(), shape);
		}

		return Register(shape.Get(), key);
	}

	/**
	* [EN]
	* Creates or reuses a sphere shape with the specified radius.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定した半径を持つ球形状を生成または再利用する。
	*/
	ScShapeHandle JoltShapePool::CreateSphereShape(Float radius)
	{
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Sphere));
		key = HashFloat(key, radius);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		JPH::ShapeRefC shape = new JPH::SphereShape(radius);

		return Register(shape.Get(), key);
	}

	/**
	* [EN]
	* Creates or reuses a capsule shape with the specified height and radius.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定した高さと半径を持つカプセル形状を生成または再利用する。
	*/
	ScShapeHandle JoltShapePool::CreateCapsuleShape(Float height, Float radius)
	{
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Capsule));
		key = HashFloat(key, height);
		key = HashFloat(key, radius);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		JPH::ShapeRefC shape = new JPH::CapsuleShape(height * 0.5f, radius);

		return Register(shape.Get(), key);
	}

	/**
	* [EN]
	* Creates or reuses a cylinder shape with the specified height and radius.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定した高さと半径を持つ円柱形状を生成または再利用する。
	*/
	ScShapeHandle JoltShapePool::CreateCylinderShape(Float height, Float radius)
	{
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Cylinder));
		key = HashFloat(key, height);
		key = HashFloat(key, radius);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		JPH::ShapeRefC shape = new JPH::CylinderShape(height * 0.5f, radius);

		return Register(shape.Get(), key);
	}

	/**
	* [EN]
	* Creates or reuses a rectangular shape in the XY plane.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* XY平面上の矩形形状を生成または再利用する。
	*/
	ScShapeHandle JoltShapePool::CreateRectShape(const Vector2& size, const Vector2& center)
	{
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Rect));
		key = HashVector2(key, size);
		key = HashVector2(key, center);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		const JPH::Vec3 halfExtent{ Max(size.x * 0.5f, JPH::cDefaultConvexRadius),Max(size.y * 0.5f, JPH::cDefaultConvexRadius),flatShapeHalfThickness_ };

		JPH::ShapeRefC shape = new JPH::BoxShape(halfExtent);

		if (center.x != 0.0f || center.y != 0.0f)
		{
			shape = new JPH::RotatedTranslatedShape(JPH::Vec3{ center.x, center.y, 0.0f }, JPH::Quat::sIdentity(), shape);
		}

		return Register(shape.Get(), key);
	}

	/**
	* [EN]
	* Creates or reuses a circular shape in the XY plane.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* XY平面上の円形状を生成または再利用する。
	*/
	ScShapeHandle JoltShapePool::CreateCircleShape(Float radius, const Vector2& center)
	{
		Uint64 key = HashCombine(0, static_cast<Uint64>(ShapeKind::Circle));
		key = HashFloat(key, radius);
		key = HashVector2(key, center);

		if (auto it = cache_.find(key); it != cache_.end())
		{
			slots_[it->second.index_].refCount_++;
			return it->second;
		}

		JPH::ShapeRefC shape = new JPH::SphereShape(radius);

		if (center.x != 0.0f || center.y != 0.0f)
		{
			shape = new JPH::RotatedTranslatedShape(JPH::Vec3{ center.x, center.y, 0.0f }, JPH::Quat::sIdentity(), shape);
		}

		return Register(shape.Get(), key);
	}

	/**
	* [EN]
	* Creates or reuses a triangle mesh shape for the specified asset,
	* with its vertices scaled per axis. Each asset and scale pair is a
	* separate shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定したアセットの頂点を軸ごとに拡縮した三角形メッシュ形状を生成
	* または再利用する。アセットとスケールの組ごとに別の形状になる。
	*/
	ScShapeHandle JoltShapePool::CreateMeshShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const DynamicArray<Uint32>& indices, const Vector3& scale)
	{
		/// [EN] The scale is baked into the vertices, so the same asset at another scale is another shape.
		/// [JP] スケールは頂点に焼き込むので、同じアセットでもスケールが違えば別の形状になる。
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

		/// [EN] Every vertex is stretched per axis by the scale.
		/// [JP] 全ての頂点を、軸ごとにスケールだけ伸ばす。
		JPH::VertexList vertices;
		vertices.reserve(positions.size());
		std::ranges::transform(positions, std::back_inserter(vertices), [&scale](const Vector3& position) { return JPH::Float3(position.x * scale.x, position.y * scale.y, position.z * scale.z); });

		/// [EN] An odd number of mirrored axes turns every triangle inside out, so two corners are swapped to keep the faces pointing outward.
		/// [JP] 反転した軸が奇数個だと全ての三角形が裏返るので、2つの角を入れ替えて面を外向きに保つ。
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

		JPH::MeshShapeSettings settings(std::move(vertices), std::move(triangles));
		settings.SetEmbedded();

		JPH::ShapeSettings::ShapeResult result = settings.Create();
		if (!result.IsValid())
		{
			SC_LOG_ERROR("メッシュ形状の生成に失敗しました: {}", result.GetError().c_str());
			return ScShapeHandle::null();
		}

		JPH::ShapeRefC shape = result.Get();

		return Register(shape.Get(), key);
	}

	/**
	* [EN]
	* Creates or reuses a convex hull shape for the specified asset,
	* with its points scaled per axis. Each asset and scale pair is a
	* separate shape.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定したアセットの点を軸ごとに拡縮した凸包形状を生成または再利用
	* する。アセットとスケールの組ごとに別の形状になる。
	*/
	ScShapeHandle JoltShapePool::CreateConvexShape(Uint32 assetID, const DynamicArray<Vector3>& positions, const Vector3& scale)
	{
		/// [EN] The scale is baked into the points, so the same asset at another scale is another shape.
		/// [JP] スケールは点に焼き込むので、同じアセットでもスケールが違えば別の形状になる。
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

		JPH::Array<JPH::Vec3> points;
		points.reserve(positions.size());
		/// [EN] Every point is stretched per axis by the scale; the hull is rebuilt from them, so a mirrored axis needs no special care.
		/// [JP] 全ての点を軸ごとにスケールだけ伸ばす。凸包は点から作り直すので、反転した軸にも特別な扱いは要らない。
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

		return Register(shape.Get(), key);
	}

	/**
	* [EN]
	* Returns the shape referenced by a valid handle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 有効なハンドルが参照する形状を返す。
	*/
	const JPH::Shape* JoltShapePool::Get(ScShapeHandle handle)const
	{
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
	* [EN]
	* Increments the reference count of a valid shape handle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 有効な形状ハンドルの参照カウントを増やす。
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
	* [EN]
	* Releases one reference and recycles the slot when no owners remain.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 参照を一つ解放し、所有者がいなくなったスロットを再利用可能にする。
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

		if (--slot.refCount_ > 0)
		{
			return;
		}

		cache_.erase(slot.cacheKey_);
		slot.shape_->Release();
		slot.shape_ = nullptr;
		++slot.generation_;
		freeIndices_.push_back(handle.index_);
	}

	/**
	* [EN]
	* Releases every shape and resets the pool.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* すべての形状を解放してプールを初期状態へ戻す。
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
	* [EN]
	* Stores a shape and associates its cache key with a new handle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 形状を格納し、キャッシュキーを新しいハンドルへ関連付ける。
	*/
	ScShapeHandle JoltShapePool::Register(const JPH::Shape* shape, Uint64 cacheKey)
	{
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
