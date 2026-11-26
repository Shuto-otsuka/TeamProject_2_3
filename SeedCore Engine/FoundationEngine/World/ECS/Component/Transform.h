#pragma once
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>

namespace SeedCore
{
	class SEEDCORE_API Transform
	{
	public:
		NODISCARD static Vector3 Vector(const Position& position);

		static void Vector(Position& position, const Vector3& value);

		NODISCARD static Vector3 Vector(const Scale& scale);

		static void Vector(Scale& scale, const Vector3& value);

		NODISCARD static Quaternion Quat(const Rotation& rotation);

		static void Quat(Rotation& rotation, const Quaternion& value);

		NODISCARD static Vector3 Euler(const Rotation& rotation);

		NODISCARD static Vector3 Degree(const Rotation& rotation);
	};
}
