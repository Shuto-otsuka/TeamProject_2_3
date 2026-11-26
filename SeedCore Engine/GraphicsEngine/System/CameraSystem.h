#pragma once
#include <FoundationEngine/Math/Halton.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <GraphicsEngine/System/SceneConstantSystem.h>
#include <GraphicsEngine/Camera/EditorCamera.h>
#include <GraphicsEngine/Camera/EditorCameraController.h>

namespace SeedCore
{
	class World;
	class GameTimer;

	enum class CameraMode
	{
		Free,

		User,
	};

	class SEEDCORE_API CameraSystem
	{
	public:
		void Update(World& world, GameTimer& timer, Float renderWidth, Float renderHeight);

		void Navigate(Float deltaTime);

	public:
		void Mode(CameraMode mode);

		CameraMode Mode()const;

	public:
		void Rect(const Vector4& rect);

		Vector4 Rect()const;

	public:
		Bool ActiveCamera()const;

		const SceneConstantBuffer& GetSceneConstantBuffer()const;

	private:
		void SyncCameraBrains(World& world, Float aspectRatio);

		void UpdateFreeCamera(World& world, GameTimer& timer, Float renderWidth, Float renderHeight);

		void UpdateUserCamera(World& world, GameTimer& timer, Float renderWidth, Float renderHeight);

	private:
		CameraMode mode_ = CameraMode::User;

		Vector4 rect_ = Vector4(0.0f, 0.0f, 1.0f, 1.0f);

		Bool hasActiveCamera_ = false;

		SceneConstantBuffer sceneConstantBuffer_{};

		Matrix previousViewProjection_ = Matrix::Identity;

		Matrix previousNonJitterViewProjection_ = Matrix::Identity;

		Vector2 jitter_ = Vector2(0.5f, 0.5f);

		Uint32 frameIndex_ = 0;

		EntityID activeBrain_;

		Vector3 lastEye_ = Vector3::Zero;

		Quaternion lastOrientation_ = Quaternion::Identity;

		Vector3 blendFromEye_ = Vector3::Zero;

		Quaternion blendFromOrientation_ = Quaternion::Identity;

		Float blendElapsed_ = 0.0f;

		Float blendDuration_ = 0.0f;

		EditorCamera freeCamera_;

		EditorCameraController freeCameraController_;
	};
}
