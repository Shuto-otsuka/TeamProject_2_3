#pragma once

namespace SeedCore
{
	enum class ShapeKind :Uint32
	{
		Box = 0,
		Sphere = 1,
		Capsule = 2,
		Cylinder = 3,
		Cone = 4,
		Ramp = 5,
		Torus = 6,
		Rect = 7,
		Circle = 8,
		Plane = 9,
		Disc = 10,
		Segment = 11,
		Arrow = 12,
	};

	enum class ShapeStyle :Uint32
	{
		Wireframe,

		Solid,
	};

	enum class ShapeSpace :Uint32
	{
		World,

		Canvas,
	};

	enum class ShapeScope :Uint32
	{
		Editor,

		Game,
	};

	struct ShapeDesc
	{
		ShapeKind kind_ = ShapeKind::Segment;

		ShapeStyle style_ = ShapeStyle::Wireframe;

		ShapeSpace space_ = ShapeSpace::World;

		ShapeScope scope_ = ShapeScope::Editor;

		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		Quaternion rotation_ = Quaternion::Identity;

		Vector3 dimensions_ = { 0.0f, 0.0f, 0.0f };

		Color color_ = { 1.0f, 1.0f, 1.0f, 1.0f };

		Float headLength_ = 0.0f;

		Float lineWidth_ = 3.0f;

		Uint32 textureID_ = 0;

		Vector2 uvScale_ = { 1.0f, 1.0f };

		Vector2 uvOffset_ = { 0.0f, 0.0f };

		Bool doubleSided_ = false;
	};

	struct ShapeList
	{
		std::span<const ShapeDesc> shapes_;

		std::span<const ShapeDesc> colliders_;
	};
}
