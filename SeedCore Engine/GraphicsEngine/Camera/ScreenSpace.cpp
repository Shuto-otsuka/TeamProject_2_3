#include <GraphicsEngine/Camera/ScreenSpace.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>

namespace SeedCore
{
	Matrix ScreenSpace::view_ = Matrix::Identity;
	Matrix ScreenSpace::projection_ = Matrix::Identity;

	Ray ScreenSpace::ScreenToWorld(const Vector2& pixelPosition)
	{
		Float normalizedX = pixelPosition.x / ScResolution::SC_CANVAS.Width;
		Float normalizedY = pixelPosition.y / ScResolution::SC_CANVAS.Height;

		Float ndcX = normalizedX * 2.0f - 1.0f;
		Float ndcY = 1.0f - normalizedY * 2.0f;

		Matrix inverseViewProjection = (view_ * projection_).Invert();

		Vector3 nearPoint = Vector3::TransformCoordinate(Vector3(ndcX, ndcY, 1.0f), inverseViewProjection);
		
		Vector3 cameraPosition = view_.Invert().Translation();

		Ray ray;
		ray.origin_ = nearPoint;
		ray.direction_ = nearPoint - cameraPosition;
		if (ray.direction_.LengthSq() > 0.0f)
		{
			ray.direction_.Normalize();
		}
		return ray;
	}

	Vector2 ScreenSpace::WorldToScreen(const Vector3& worldPosition)
	{
		Vector3 clipPosition = Vector3::TransformCoordinate(worldPosition, view_ * projection_);

		Float pixelX = (clipPosition.x * 0.5f + 0.5f) * ScResolution::SC_CANVAS.Width;
		Float pixelY = (1.0f - (clipPosition.y * 0.5f + 0.5f)) * ScResolution::SC_CANVAS.Height;
		return Vector2(pixelX, pixelY);
	}

	void ScreenSpace::SetCurrentView(const Matrix& view, const Matrix& projection)
	{
		view_ = view;
		projection_ = projection;
	}
}
