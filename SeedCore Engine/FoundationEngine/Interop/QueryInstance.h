#pragma once
#include <FoundationEngine/Interop/ColliderInstance.h>

namespace SeedCore
{
	enum class QueryKind :Uint32
	{
		Raycast,

		Spherecast,

		Boxcast,

		Overlap,

		Raycast2D,

		Circlecast2D,

		Boxcast2D,

		Overlap2D,
	};

	struct QueryDesc
	{
		QueryKind kind_ = QueryKind::Raycast;

		Vector3 origin_ = { 0.0f, 0.0f, 0.0f };

		Vector3 direction_ = { 0.0f, 0.0f, 0.0f };

		Float distance_ = 0.0f;

		Float radius_ = 0.0f;

		ColliderKind shape_ = ColliderKind::Box;

		Vector3 dimensions_ = { 0.0f, 0.0f, 0.0f };

		Quaternion rotation_ = Quaternion::Identity;

		Bool hit_ = false;

		Vector3 hitPoint_ = { 0.0f, 0.0f, 0.0f };

		Vector3 hitNormal_ = { 0.0f, 0.0f, 0.0f };

		Float hitDistance_ = 0.0f;
	};

	class SEEDCORE_API QueryInstance
	{
	public:
		void Add(const QueryDesc& query);

		NODISCARD std::span<const QueryDesc> Queries()const;

		void Clear();

	private:
		DynamicArray<QueryDesc> queries_;
	};
}
