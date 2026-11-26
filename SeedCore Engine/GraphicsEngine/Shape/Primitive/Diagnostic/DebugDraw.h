#pragma once
#include <FoundationEngine/Interop/RenderInstance.h>

namespace SeedCore
{
	class SEEDCORE_API DebugDraw
	{
	public:
		DebugDraw(RenderInstance& renderInstance);

		~DebugDraw() = default;

	public:
		void Line(const Vector3& start, const Vector3& end, const Color& color);

		void Arrow(const Vector3& start, const Vector3& end, const Color& color, Float headLength = 0.0f);

		void Box(const Vector3& center, const Quaternion& rotation, const Vector3& size, const Color& color);

		void Sphere(const Vector3& center, Float radius, const Color& color);

		void Capsule(const Vector3& center, const Quaternion& rotation, Float radius, Float height, const Color& color);

		void Cylinder(const Vector3& center, const Quaternion& rotation, Float radius, Float height, const Color& color);

	private:
		RenderInstance& renderInstance_;
	};
}
