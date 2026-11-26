#include <GraphicsEngine/System/CameraSystem.h>
#include <GraphicsEngine/Camera/Camera.h>
#include <GraphicsEngine/Camera/CameraBrain.h>
#include <GraphicsEngine/Camera/ScreenSpace.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/Time/GameTimer.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>

namespace SeedCore
{
	void CameraSystem::Update(World& world, GameTimer& timer, Float renderWidth, Float renderHeight)
	{
		SyncCameraBrains(world, renderWidth / renderHeight);

		if (mode_ == CameraMode::Free)
		{
			UpdateFreeCamera(world, timer, renderWidth, renderHeight);
		}
		else
		{
			UpdateUserCamera(world, timer, renderWidth, renderHeight);
		}
	}

	void CameraSystem::Navigate(Float deltaTime)
	{
		freeCameraController_.Update(freeCamera_, deltaTime);
	}

	void CameraSystem::Mode(CameraMode mode)
	{
		mode_ = mode;
	}

	CameraMode CameraSystem::Mode()const
	{
		return mode_;
	}

	void CameraSystem::Rect(const Vector4& rect)
	{
		if (rect.z <= 0.0f || rect.w <= 0.0f)
		{
			return;
		}

		rect_ = rect;
	}

	Vector4 CameraSystem::Rect()const
	{
		return rect_;
	}

	Bool CameraSystem::ActiveCamera()const
	{
		return hasActiveCamera_;
	}

	const SceneConstantBuffer& CameraSystem::GetSceneConstantBuffer()const
	{
		return sceneConstantBuffer_;
	}

	void CameraSystem::SyncCameraBrains(World& world, Float aspectRatio)
	{
		Float fieldOfView = 60.0f;
		Bool lensFound = false;
		Query<Read<Active>, Read<Camera>> lensQuery(world);
		lensQuery.ForEach([&](const Active& active, const Camera& camera)
			{
				if (lensFound || !active.active_ || !camera.isActive_)
				{
					return;
				}
				lensFound = true;
				fieldOfView = camera.fieldOfView_;
			});

		Query<Write<CameraBrain>, Write<Rotation>> syncQuery(world);
		syncQuery.ForEach([&](CameraBrain& brain, Rotation& rotation)
			{
				brain.fieldOfView_ = fieldOfView;
				brain.aspectRatio_ = aspectRatio;

				Quaternion rotationQuaternion = Transform::Quat(rotation);
				Bool directionChanged = brain.synced_ && (brain.direction_ - brain.syncedDirection_).LengthSq() > 1e-10f;
				Bool rotationChanged = !brain.synced_ || rotationQuaternion != brain.syncedRotation_;

				if (directionChanged)
				{
					Vector3 direction = brain.direction_;
					if (direction.LengthSq() < 1e-8f)
					{
						direction = brain.syncedDirection_;
					}
					direction.Normalize();
					brain.direction_ = direction;

					Vector3 rotationDegree = Transform::Degree(rotation);
					rotationDegree.x = ToDegrees(Asin(Clamp(-direction.y, -1.0f, 1.0f)));
					if (direction.x * direction.x + direction.z * direction.z > 1e-8f)
					{
						rotationDegree.y = ToDegrees(Atan2(direction.x, direction.z));
					}

					Quaternion synchronizedRotation = Quaternion::CreateFromYawPitchRoll(ToRadians(rotationDegree.y), ToRadians(rotationDegree.x), ToRadians(rotationDegree.z));
					Transform::Quat(rotation, synchronizedRotation);
					rotationQuaternion = synchronizedRotation;
				}
				else if (rotationChanged)
				{
					Matrix rotationMatrix = Matrix::CreateFromQuaternion(rotationQuaternion);
					Vector3 direction = Vector3::TransformDirection(Vector3::Forward, rotationMatrix);
					direction.Normalize();
					brain.direction_ = direction;
				}

				brain.syncedDirection_ = brain.direction_;
				brain.syncedRotation_ = rotationQuaternion;
				brain.synced_ = true;
			});
	}

	void CameraSystem::UpdateFreeCamera(World& world, GameTimer& timer, Float renderWidth, Float renderHeight)
	{
		hasActiveCamera_ = false;

		Query<Read<Active>, Read<Camera>> cameraQuery(world);
		cameraQuery.ForEach([&](const Active& active, const Camera& camera)
			{
				if (hasActiveCamera_ || !active.active_ || !camera.isActive_)
				{
					return;
				}
				hasActiveCamera_ = true;

				freeCamera_.Fov(camera.fieldOfView_);
				freeCamera_.Near(camera.nearPlane_);
				freeCamera_.Far(camera.farPlane_);
			});

		if (!hasActiveCamera_)
		{
			return;
		}

		freeCamera_.Resize(renderWidth, renderHeight);
		freeCamera_.Tick(timer.ScaledDeltaTime());

		sceneConstantBuffer_.view_ = freeCamera_.View();
		sceneConstantBuffer_.inverseView_ = freeCamera_.InverseView();
		sceneConstantBuffer_.projection_ = freeCamera_.Projection();
		sceneConstantBuffer_.inverseProjection_ = freeCamera_.InverseProjection();
		sceneConstantBuffer_.nonJitterProjection_ = freeCamera_.NonJitterProjection();
		sceneConstantBuffer_.currentViewProjection_ = freeCamera_.CurrentViewProjection();
		sceneConstantBuffer_.previousViewProjection_ = previousViewProjection_;
		sceneConstantBuffer_.inverseViewProjection_ = freeCamera_.InverseViewProjection();
		sceneConstantBuffer_.nonJitterViewProjection_ = freeCamera_.NonJitterViewProjection();
		sceneConstantBuffer_.previousNonJitterViewProjection_ = previousNonJitterViewProjection_;
		sceneConstantBuffer_.cameraPosition_ = Vector4(freeCamera_.Eye(), 1.0f);
		sceneConstantBuffer_.cameraFocus_ = Vector4(freeCamera_.Focus(), 1.0f);
		sceneConstantBuffer_.fieldOfView_ = freeCamera_.Fov();
		sceneConstantBuffer_.nearPlane_ = freeCamera_.Near();
		sceneConstantBuffer_.farPlane_ = freeCamera_.Far();
		sceneConstantBuffer_.totalTime_ = timer.ScaledTotalTime();
		sceneConstantBuffer_.deltaTime_ = timer.ScaledDeltaTime();
		sceneConstantBuffer_.screenSize_ = Vector2(renderWidth, renderHeight);
		sceneConstantBuffer_.inverseScreenSize_ = Vector2(1.0f / renderWidth, 1.0f / renderHeight);
		sceneConstantBuffer_.displaySize_ = sceneConstantBuffer_.screenSize_;

		previousViewProjection_ = freeCamera_.CurrentViewProjection();
		previousNonJitterViewProjection_ = freeCamera_.NonJitterViewProjection();
	}

	void CameraSystem::UpdateUserCamera(World& world, GameTimer& timer, Float renderWidth, Float renderHeight)
	{
		hasActiveCamera_ = false;

		const Camera* activeCamera = nullptr;
		Vector3 eye = Vector3::Zero;
		Quaternion orientation = Quaternion::Identity;

		Query<Read<Active>, Read<Camera>, Read<Position>, Read<Rotation>> query(world);
		query.ForEach([&](const Active& active, const Camera& camera, const Position& position, const Rotation& rotation)
			{
				if (hasActiveCamera_ || !active.active_ || !camera.isActive_)
				{
					return;
				}
				hasActiveCamera_ = true;
				activeCamera = &camera;
				eye = Vector3(position.x_, position.y_, position.z_);
				orientation = Transform::Quat(rotation);
			});

		if (!hasActiveCamera_)
		{
			return;
		}

		CameraBrain* activeBrain = nullptr;
		EntityID activeBrainEntity;
		Bool activeBrainCut = false;
		Vector3 brainEye = Vector3::Zero;
		Quaternion brainOrientation = Quaternion::Identity;

		Query<Read<Active>, Write<CameraBrain>, Read<Position>, Read<Rotation>> brainQuery(world);
		brainQuery.ForEach([&](EntityID entity, const Active& active, CameraBrain& brain, const Position& position, const Rotation& rotation)
			{
				Bool brainCut = brain.cut_;
				brain.cut_ = false;

				if (!active.active_)
				{
					return;
				}
				if (activeBrain && (brain.weight_ < activeBrain->weight_ || (brain.weight_ == activeBrain->weight_ && entity != activeBrain_)))
				{
					return;
				}

				activeBrain = &brain;
				activeBrainEntity = entity;
				activeBrainCut = brainCut;
				brainEye = Vector3(position.x_, position.y_, position.z_);
				Quaternion shakeRotation = Quaternion::CreateFromYawPitchRoll(ToRadians(brain.shakeAngles_.y), ToRadians(brain.shakeAngles_.x), ToRadians(brain.shakeAngles_.z));
				brainOrientation = Transform::Quat(rotation) * shakeRotation;
			});

		Bool cut = false;
		if (activeBrain)
		{
			if (activeBrainEntity != activeBrain_)
			{
				Bool blend = timer.Playing() && activeBrain_ != EntityID() && activeBrain->blendTime_ > 0.0f;
				blendFromEye_ = lastEye_;
				blendFromOrientation_ = lastOrientation_;
				blendElapsed_ = 0.0f;
				blendDuration_ = blend ? activeBrain->blendTime_ : 0.0f;
				activeBrain_ = activeBrainEntity;
			}
			if (activeBrainCut)
			{
				cut = true;
				blendDuration_ = 0.0f;
			}
			eye = brainEye;
			orientation = brainOrientation;
		}
		else
		{
			activeBrain_ = EntityID();
			blendDuration_ = 0.0f;
		}

		if (blendElapsed_ < blendDuration_)
		{
			blendElapsed_ += timer.ScaledDeltaTime();
			Float blendRatio = Clamp(blendElapsed_ / blendDuration_, 0.0f, 1.0f);
			blendRatio = blendRatio * blendRatio * (3.0f - 2.0f * blendRatio);
			eye = Vector3::Lerp(blendFromEye_, eye, blendRatio);
			orientation = Quaternion::Slerp(blendFromOrientation_, orientation, blendRatio);
		}
		lastEye_ = eye;
		lastOrientation_ = orientation;

		const Vector3 forward = Vector3::Transform(Vector3::Forward, orientation);
		const Vector3 focus = eye + forward;
		const Vector3 up = Vector3::Transform(Vector3::Up, orientation);

		const Float aspectRatio = renderWidth / renderHeight;
		const Matrix view = Matrix::CreateViewLookAt(eye, focus, up);

		Matrix nonJitterProjection = Matrix::CreatePerspectiveFov(ToRadians(activeCamera->fieldOfView_), aspectRatio, activeCamera->farPlane_, activeCamera->nearPlane_);

		jitter_ = Halton23Jitter(frameIndex_);
		++frameIndex_;

		Matrix projection = nonJitterProjection;
		if (jitter_.LengthSq() > 0.0f)
		{
			projection._31 += (jitter_.x * 2.0f) / renderWidth;
			projection._32 -= (jitter_.y * 2.0f) / renderHeight;
		}

		const Matrix viewProjection = view * projection;
		const Matrix nonJitterViewProjection = view * nonJitterProjection;

		if (cut)
		{
			previousViewProjection_ = viewProjection;
			previousNonJitterViewProjection_ = nonJitterViewProjection;
		}

		sceneConstantBuffer_.view_ = view;
		sceneConstantBuffer_.inverseView_ = view.Invert();
		sceneConstantBuffer_.projection_ = projection;
		sceneConstantBuffer_.inverseProjection_ = projection.Invert();
		sceneConstantBuffer_.nonJitterProjection_ = nonJitterProjection;
		sceneConstantBuffer_.currentViewProjection_ = viewProjection;
		sceneConstantBuffer_.previousViewProjection_ = previousViewProjection_;
		sceneConstantBuffer_.inverseViewProjection_ = viewProjection.Invert();
		sceneConstantBuffer_.nonJitterViewProjection_ = nonJitterViewProjection;
		sceneConstantBuffer_.previousNonJitterViewProjection_ = previousNonJitterViewProjection_;
		sceneConstantBuffer_.cameraPosition_ = Vector4(eye, 1.0f);
		sceneConstantBuffer_.cameraFocus_ = Vector4(focus, 1.0f);
		sceneConstantBuffer_.fieldOfView_ = activeCamera->fieldOfView_;
		sceneConstantBuffer_.nearPlane_ = activeCamera->nearPlane_;
		sceneConstantBuffer_.farPlane_ = activeCamera->farPlane_;
		sceneConstantBuffer_.totalTime_ = timer.ScaledTotalTime();
		sceneConstantBuffer_.deltaTime_ = timer.ScaledDeltaTime();
		sceneConstantBuffer_.screenSize_ = Vector2(renderWidth, renderHeight);
		sceneConstantBuffer_.inverseScreenSize_ = Vector2(1.0f / renderWidth, 1.0f / renderHeight);
		sceneConstantBuffer_.displaySize_ = sceneConstantBuffer_.screenSize_;

		previousViewProjection_ = viewProjection;
		previousNonJitterViewProjection_ = nonJitterViewProjection;

		ScreenSpace::SetCurrentView(view, nonJitterProjection);
	}
}
