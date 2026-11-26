#pragma once

namespace SeedCore
{
	class PreviewCamera;

	class SEEDCORE_API PreviewCameraController
	{
	public:
		void Update(PreviewCamera& camera, Float deltaTime);

		void RotateSpeed(Float speed);

		void ZoomSpeed(Float speed);

		void PanSpeed(Float speed);

		Float RotateSpeed()const;

		Float ZoomSpeed()const;

		Float PanSpeed()const;

	private:
		Float yaw_ = 0.0f;

		Float pitch_ = 0.0f;

		Float distance_ = 1.0f;

		Bool initialized_ = false;

		Float rotateSpeed_ = 0.3f;

		Float zoomSpeed_ = 0.1f;

		Float panSpeed_ = 0.0025f;
	};
}
