#pragma once

namespace SeedCore
{
	class CameraSystem;

	class SEEDCORE_API ScreenSpace
	{
		friend class CameraSystem;

	public:
		static Ray ScreenToWorld(const Vector2& pixelPosition);

		static Vector2 WorldToScreen(const Vector3& worldPosition);

	private:
		static void SetCurrentView(const Matrix& view, const Matrix& projection);

	private:
		static Matrix view_;

		static Matrix projection_;
	};
}
