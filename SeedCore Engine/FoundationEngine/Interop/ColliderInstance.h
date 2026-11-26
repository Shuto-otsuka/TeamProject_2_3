#pragma once

namespace SeedCore
{
	enum class ColliderKind :Uint32
	{
		Box = 0,
		Sphere = 1,
		Capsule = 2,
		Cylinder = 3,
		Rect = 7,
		Circle = 8,
	};

	struct ColliderDesc
	{
		ColliderKind kind_ = ColliderKind::Box;

		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		Quaternion rotation_ = Quaternion::Identity;

		Vector3 dimensions_ = { 0.0f, 0.0f, 0.0f };

		Color color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	};
}
