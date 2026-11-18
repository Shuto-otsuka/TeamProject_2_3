#include <GraphicsEngine/Camera/EditorCamera.h>
#include <FoundationEngine/Math/Halton.h>

namespace SeedCore
{
	void EditorCamera::Tick(Float elapsedTime)
	{
		previousViewProjection_ = currentViewProjection_;
		previousNonJitterViewProjection_ = nonJitterViewProjection_;

		if (focusing_)
		{
			focusElapsed_ += elapsedTime;
			Float t = Clamp(focusElapsed_ / focusDuration_, 0.0f, 1.0f);
			Float smoothT = t * t * (3.0f - 2.0f * t);

			eye_ = Vector3::Lerp(focusStartEye_, focusTargetEye_, smoothT);
			focus_ = Vector3::Lerp(focusStartFocus_, focusTargetFocus_, smoothT);

			if (t >= 1.0f)
			{
				focusing_ = false;
			}
		}

		forward_ = focus_ - eye_;
		if (forward_.LengthSq() < 1e-4)
		{
			forward_ = Vector3::Forward;
		}
		else
		{
			forward_.Normalize();
		}

		right_ = forward_.Cross(up_);
		if (right_.LengthSq() < 1e-4)
		{
			right_ = Vector3::Right;
		}
		else
		{
			right_.Normalize();
		}

		view_ = Matrix::CreateViewLookAt(eye_, focus_, up_);

		aspectRatio_ = width_ / height_;
		nonJitterProjection_ = Matrix::CreatePerspectiveFov(ToRadians(Max(0.001f, fieldOfView_)), aspectRatio_, farPlane_, nearPlane_);

		jitter_ = Halton23Jitter(frameIndex_);
		++frameIndex_;

		projection_ = nonJitterProjection_;

		if (jitter_.LengthSq() > 0.0f)
		{
			projection_._31 += (jitter_.x * 2.0f) / static_cast<Float>(width_);
			projection_._32 -= (jitter_.y * 2.0f) / static_cast<Float>(height_);
		}

		currentViewProjection_ = view_ * projection_;

		nonJitterViewProjection_ = view_ * nonJitterProjection_;

		inverseView_ = view_.Invert();

		inverseProjection_ = projection_.Invert();

		inverseViewProjection_ = currentViewProjection_.Invert();
	}

	void EditorCamera::Resize(Float width, Float height)
	{
		width_ = width;
		height_ = height;
	}

	void EditorCamera::Eye(Vector3 eye)
	{
		eye_ = eye;
	}

	void EditorCamera::Focus(Vector3 focus)
	{
		focus_ = focus;
	}

	void EditorCamera::FocusOn(Vector3 targetPosition, Float radius)
	{
		if (!std::isfinite(targetPosition.x) || !std::isfinite(targetPosition.y) || !std::isfinite(targetPosition.z))
		{
			return;
		}
		if (!std::isfinite(radius))
		{
			radius = 0.0f;
		}

		Vector3 offset = eye_ - focus_;
		Float currentDistance = offset.Length();
		if (currentDistance < 1e-6f)
		{
			offset = Vector3(0.0f, 0.0f, -1.0f);
			currentDistance = 1.0f;
		}
		Vector3 direction = offset / currentDistance;

		Float distance = currentDistance;
		if (radius > 0.0f)
		{
			Float halfFovRadians = ToRadians(Max(fieldOfView_, 1.0f)) * 0.5f;
			distance = radius / std::sin(halfFovRadians);
		}

		focusStartEye_ = eye_;
		focusStartFocus_ = focus_;
		focusTargetFocus_ = targetPosition;
		focusTargetEye_ = targetPosition + direction * distance;

		focusElapsed_ = 0.0f;
		focusing_ = true;
	}

	void EditorCamera::Up(Vector3 up)
	{
		up_ = up;
	}

	void EditorCamera::Near(Float nearPlane)
	{
		nearPlane_ = nearPlane;
	}

	void EditorCamera::Far(Float farPlane)
	{
		farPlane_ = farPlane;
	}

	void EditorCamera::Fov(Float fieldOfView)
	{
		fieldOfView_ = fieldOfView;
	}

	Vector3 EditorCamera::Eye()const
	{
		return eye_;
	}

	Vector3 EditorCamera::Focus()const
	{
		return focus_;
	}

	Vector3 EditorCamera::Up()const
	{
		return up_;
	}

	Vector3 EditorCamera::Forward()const
	{
		return forward_;
	}

	Vector3 EditorCamera::Right()const
	{
		return right_;
	}

	Float EditorCamera::Near()const
	{
		return nearPlane_;
	}

	Float EditorCamera::Far()const
	{
		return farPlane_;
	}

	Float EditorCamera::Fov()const
	{
		return fieldOfView_;
	}

	Float EditorCamera::AspectRatio()const
	{
		return aspectRatio_;
	}

	Float EditorCamera::Width()const
	{
		return width_;
	}

	Float EditorCamera::Height()const
	{
		return height_;
	}

	Matrix EditorCamera::View()const
	{
		return view_;
	}

	Matrix EditorCamera::InverseView()const
	{
		return inverseView_;
	}

	Matrix EditorCamera::Projection()const
	{
		return projection_;
	}

	Matrix EditorCamera::InverseProjection()const
	{
		return inverseProjection_;
	}

	Matrix EditorCamera::NonJitterProjection()const
	{
		return nonJitterProjection_;
	}

	Matrix EditorCamera::CurrentViewProjection()const
	{
		return currentViewProjection_;
	}

	Matrix EditorCamera::PreviousViewProjection()const
	{
		return previousViewProjection_;
	}

	Matrix EditorCamera::PreviousNonJitterViewProjection()const
	{
		return previousNonJitterViewProjection_;
	}

	Matrix EditorCamera::InverseViewProjection()const
	{
		return inverseViewProjection_;
	}

	Matrix EditorCamera::NonJitterViewProjection()const
	{
		return nonJitterViewProjection_;
	}
}