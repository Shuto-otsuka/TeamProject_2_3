#include <GraphicsEngine/Shape/Primitive/Diagnostic/DebugDraw.h>
#include <FoundationEngine/Interop/RenderInstance.h>

namespace SeedCore
{
	DebugDraw::DebugDraw(RenderInstance& renderInstance) :renderInstance_(renderInstance)
	{
		/// No Code
	}

	void DebugDraw::Line(const Vector3& start, const Vector3& end, const Color& color)
	{
#if SC_DEBUG || SC_DEVELOP
		ShapeDesc shape{};
		shape.kind_ = ShapeKind::Segment;
		shape.style_ = ShapeStyle::Wireframe;
		shape.space_ = ShapeSpace::World;
		shape.scope_ = ShapeScope::Game;
		shape.position_ = start;
		shape.rotation_ = Quaternion::Identity;
		shape.dimensions_ = end - start;
		shape.color_ = color;
		renderInstance_.Add(shape);
#endif
	}

	void DebugDraw::Arrow(const Vector3& start, const Vector3& end, const Color& color, Float headLength)
	{
#if SC_DEBUG || SC_DEVELOP
		Vector3 direction = end - start;
		Float length = direction.Length();
		if (length < 1e-5f)
		{
			return;
		}
		direction /= length;

		Float arrowHeadLength = length * 0.2f;
		if (headLength > 0.0f)
		{
			arrowHeadLength = Min(arrowHeadLength, headLength);
		}
		Float headRadius = arrowHeadLength * 0.4f;
		Float shaftRadius = headRadius / 3.0f;
		Float shaftLength = length - arrowHeadLength;

		Quaternion rotation = Quaternion::FromToRotation(Vector3::UnitY, direction);

		ShapeDesc shaft{};
		shaft.kind_ = ShapeKind::Cylinder;
		shaft.style_ = ShapeStyle::Solid;
		shaft.space_ = ShapeSpace::World;
		shaft.scope_ = ShapeScope::Game;
		shaft.position_ = start + direction * (shaftLength * 0.5f);
		shaft.rotation_ = rotation;
		shaft.dimensions_ = Vector3(shaftRadius, shaftLength * 0.5f, 0.0f);
		shaft.color_ = color;
		renderInstance_.Add(shaft);

		ShapeDesc head{};
		head.kind_ = ShapeKind::Cone;
		head.style_ = ShapeStyle::Solid;
		head.space_ = ShapeSpace::World;
		head.scope_ = ShapeScope::Game;
		head.position_ = start + direction * (shaftLength + arrowHeadLength * 0.5f);
		head.rotation_ = rotation;
		head.dimensions_ = Vector3(headRadius, arrowHeadLength * 0.5f, 0.0f);
		head.color_ = color;
		renderInstance_.Add(head);
#endif
	}

	void DebugDraw::Box(const Vector3& center, const Quaternion& rotation, const Vector3& size, const Color& color)
	{
#if SC_DEBUG || SC_DEVELOP
		ShapeDesc shape{};
		shape.kind_ = ShapeKind::Box;
		shape.style_ = ShapeStyle::Wireframe;
		shape.space_ = ShapeSpace::World;
		shape.scope_ = ShapeScope::Game;
		shape.position_ = center;
		shape.rotation_ = rotation;
		shape.dimensions_ = Vector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
		shape.color_ = color;
		renderInstance_.Add(shape);
#endif
	}

	void DebugDraw::Sphere(const Vector3& center, Float radius, const Color& color)
	{
#if SC_DEBUG || SC_DEVELOP
		ShapeDesc shape{};
		shape.kind_ = ShapeKind::Sphere;
		shape.style_ = ShapeStyle::Wireframe;
		shape.space_ = ShapeSpace::World;
		shape.scope_ = ShapeScope::Game;
		shape.position_ = center;
		shape.rotation_ = Quaternion::Identity;
		shape.dimensions_ = Vector3(radius, 0.0f, 0.0f);
		shape.color_ = color;
		renderInstance_.Add(shape);
#endif
	}

	void DebugDraw::Capsule(const Vector3& center, const Quaternion& rotation, Float radius, Float height, const Color& color)
	{
#if SC_DEBUG || SC_DEVELOP
		ShapeDesc shape{};
		shape.kind_ = ShapeKind::Capsule;
		shape.style_ = ShapeStyle::Wireframe;
		shape.space_ = ShapeSpace::World;
		shape.scope_ = ShapeScope::Game;
		shape.position_ = center;
		shape.rotation_ = rotation;
		shape.dimensions_ = Vector3(radius, height * 0.5f, 0.0f);
		shape.color_ = color;
		renderInstance_.Add(shape);
#endif
	}

	void DebugDraw::Cylinder(const Vector3& center, const Quaternion& rotation, Float radius, Float height, const Color& color)
	{
#if SC_DEBUG || SC_DEVELOP
		ShapeDesc shape{};
		shape.kind_ = ShapeKind::Cylinder;
		shape.style_ = ShapeStyle::Wireframe;
		shape.space_ = ShapeSpace::World;
		shape.scope_ = ShapeScope::Game;
		shape.position_ = center;
		shape.rotation_ = rotation;
		shape.dimensions_ = Vector3(radius, height * 0.5f, 0.0f);
		shape.color_ = color;
		renderInstance_.Add(shape);
#endif
	}
}
