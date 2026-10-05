#include <GraphicsEngine/System/CameraSystem.h>
#include <GraphicsEngine/Camera/Camera.h>
#include <GraphicsEngine/Camera/CameraBrain.h>
#include <GraphicsEngine/Camera/ScreenSpace.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Active.h>
#include <FoundationEngine/Time/GameTimer.h>

namespace SeedCore
{
	/**
	* [EN]
	* Resolves this frame's camera for a render extent of
	* renderWidth x renderHeight and rebuilds the scene constant buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* renderWidth x renderHeight の描画サイズに対して今フレームのカメラを
	* 解決し、シーン定数バッファを作り直す。
	*/
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

	/**
	* [EN]
	* Feeds the Editor's mouse/keyboard input to the free-fly camera.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Editor のマウス/キーボード入力をフリーカメラへ渡す。
	*/
	void CameraSystem::Navigate(Float deltaTime)
	{
		freeCameraController_.Update(freeCamera_, deltaTime);
	}

	/**
	* [EN]
	* Selects which camera the game view renders from.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームビューがどのカメラで描画するかを選ぶ。
	*/
	void CameraSystem::Mode(CameraMode mode)
	{
		mode_ = mode;
	}

	/**
	* [EN]
	* Returns which camera the game view renders from.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームビューがどのカメラで描画しているかを返す。
	*/
	CameraMode CameraSystem::Mode()const
	{
		return mode_;
	}

	/**
	* [EN]
	* Sets where the game image is displayed on the desktop, in desktop
	* pixels (x = left, y = top, z = width, w = height). A degenerate size
	* is ignored so the divisions in ScreenSpace stay finite.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲーム画像がデスクトップ上のどこに表示されているかを、
	* デスクトップのピクセル座標(x = 左、y = 上、z = 幅、w = 高さ)で
	* 設定する。ScreenSpace の除算が有限に収まるよう、大きさが0以下の
	* ものは無視する。
	*/
	void CameraSystem::Rect(const Vector4& rect)
	{
		if (rect.z <= 0.0f || rect.w <= 0.0f)
		{
			return;
		}

		rect_ = rect;
	}

	/**
	* [EN]
	* Returns where the game image is displayed on the desktop, in
	* desktop pixels (x = left, y = top, z = width, w = height).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲーム画像がデスクトップ上のどこに表示されているかを、
	* デスクトップのピクセル座標(x = 左、y = 上、z = 幅、w = 高さ)で返す。
	*/
	Vector4 CameraSystem::Rect()const
	{
		return rect_;
	}

	/**
	* [EN]
	* Whether an active Camera was found by the last Update().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直前の Update() でアクティブな Camera が見つかったかどうか。
	*/
	Bool CameraSystem::ActiveCamera()const
	{
		return hasActiveCamera_;
	}

	/**
	* [EN]
	* Returns the scene constant buffer built by the last Update().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直前の Update() で作られたシーン定数バッファを返す。
	*/
	const SceneConstantBuffer& CameraSystem::GetSceneConstantBuffer()const
	{
		return sceneConstantBuffer_;
	}

	/**
	* [EN]
	* Pushes the active lens and aspect ratio into every CameraBrain and
	* keeps each brain's direction_ and its Rotation in sync, whichever
	* side was edited.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アクティブなレンズとアスペクト比を全 CameraBrain へ反映し、
	* どちらが編集されたかに応じて各 brain の direction_ と Rotation を
	* 同期させる。
	*/
	void CameraSystem::SyncCameraBrains(World& world, Float aspectRatio)
	{
		/// [EN] The first active Camera supplies the lens every brain reports.
		/// [JP] 最初に見つかったアクティブな Camera のレンズを、全 brain に持たせる。
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

				Quaternion rotationQuaternion = rotation.Quat();
				Bool directionChanged = brain.synced_ && (brain.direction_ - brain.syncedDirection_).LengthSquared() > 1e-10f;
				Bool rotationChanged = !brain.synced_ || rotationQuaternion != brain.syncedRotation_;

				/// [EN] direction_ was edited: rebuild pitch/yaw from it, keeping the current roll.
				/// [JP] direction_ が編集された場合は、現在のロールを保ったまま、そこからピッチ/ヨーを作り直す。
				if (directionChanged)
				{
					Vector3 direction = brain.direction_;
					if (direction.LengthSquared() < 1e-8f)
					{
						direction = brain.syncedDirection_;
					}
					direction.Normalize();
					brain.direction_ = direction;

					Vector3 rotationDegree = rotation.Degree();
					rotationDegree.x = ToDegrees(Asin(Clamp(-direction.y, -1.0f, 1.0f)));
					if (direction.x * direction.x + direction.z * direction.z > 1e-8f)
					{
						rotationDegree.y = ToDegrees(Atan2(direction.x, direction.z));
					}

					Quaternion synchronizedRotation = Quaternion::CreateFromYawPitchRoll(ToRadians(rotationDegree.y), ToRadians(rotationDegree.x), ToRadians(rotationDegree.z));
					rotation.x_ = synchronizedRotation.x;
					rotation.y_ = synchronizedRotation.y;
					rotation.z_ = synchronizedRotation.z;
					rotation.w_ = synchronizedRotation.w;
					rotationQuaternion = synchronizedRotation;
				}
				/// [EN] Rotation was edited: derive direction_ from its forward axis.
				/// [JP] Rotation が編集された場合は、その前方軸から direction_ を求める。
				else if (rotationChanged)
				{
					Matrix rotationMatrix = Matrix::CreateFromQuaternion(rotationQuaternion);
					Vector3 direction = Vector3::TransformNormal(Vector3::Forward, rotationMatrix);
					direction.Normalize();
					brain.direction_ = direction;
				}

				brain.syncedDirection_ = brain.direction_;
				brain.syncedRotation_ = rotationQuaternion;
				brain.synced_ = true;
			});
	}

	/**
	* [EN]
	* Builds the scene constant buffer from the Editor's free-fly camera,
	* taking only the lens settings from the game's active Camera.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Editor のフリーカメラからシーン定数バッファを作る。ゲームの
	* アクティブな Camera からはレンズ設定だけを取る。
	*/
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
		sceneConstantBuffer_.cameraPosition_ = Vector4(freeCamera_.Eye().x, freeCamera_.Eye().y, freeCamera_.Eye().z, 1.0f);
		sceneConstantBuffer_.cameraFocus_ = Vector4(freeCamera_.Focus().x, freeCamera_.Focus().y, freeCamera_.Focus().z, 1.0f);
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

	/**
	* [EN]
	* Builds the scene constant buffer from the game's active Camera,
	* blended through the highest-weight CameraBrain, and publishes it to
	* ScreenSpace.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームのアクティブな Camera から、最も weight の高い CameraBrain に
	* よるブレンドを通してシーン定数バッファを作り、ScreenSpace へ公開する。
	*/
	void CameraSystem::UpdateUserCamera(World& world, GameTimer& timer, Float renderWidth, Float renderHeight)
	{
		hasActiveCamera_ = false;

		/// [EN] The first active Camera supplies the lens and the default eye/orientation.
		/// [JP] 最初に見つかったアクティブな Camera が、レンズと既定の視点/向きを決める。
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
				orientation = rotation.Quat();
			});

		if (!hasActiveCamera_)
		{
			return;
		}

		/// [EN] The highest-weight active brain overrides eye/orientation; on a tie the current brain wins.
		/// [JP] weight が最も高いアクティブな brain が視点/向きを上書きする。同じ weight なら現在の brain を優先する。
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
				brainOrientation = rotation.Quat() * shakeRotation;
			});

		/// [EN] Switching brains starts a blend from last frame's eye/orientation; a cut drops any blend and resets motion history.
		/// [JP] brain が切り替わったら前フレームの視点/向きからブレンドを始める。カットはブレンドを打ち切り、モーション履歴もリセットする。
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

		/// [EN] Smoothstep blend from the previous brain's pose.
		/// [JP] 前の brain の姿勢から smoothstep でブレンドする。
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
		const Matrix view = Matrix::CreateLookAt(eye, focus, up);

		Matrix nonJitterProjection = Matrix::CreatePerspectiveFieldOfView(ToRadians(activeCamera->fieldOfView_), aspectRatio, activeCamera->nearPlane_, activeCamera->farPlane_);
		nonJitterProjection._33 = activeCamera->nearPlane_ / (activeCamera->nearPlane_ - activeCamera->farPlane_);
		nonJitterProjection._43 = (activeCamera->farPlane_ * activeCamera->nearPlane_) / (activeCamera->farPlane_ - activeCamera->nearPlane_);

		/// [EN] Offset the projection by this frame's sub-pixel jitter for TAA/upscaling.
		/// [JP] TAA/アップスケール用に、今フレームのサブピクセルジッター分だけ射影をずらす。
		jitter_ = Halton23Jitter(frameIndex_);
		++frameIndex_;

		Matrix projection = nonJitterProjection;
		if (jitter_.LengthSquared() > 0.0f)
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
		sceneConstantBuffer_.cameraPosition_ = Vector4(eye.x, eye.y, eye.z, 1.0f);
		sceneConstantBuffer_.cameraFocus_ = Vector4(focus.x, focus.y, focus.z, 1.0f);
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

		/// [EN] nonJitterProjection, not the TAA-jittered projection
		///      above - gameplay screen<->world picking should use
		///      the camera's true projection, not the render-only
		///      sub-pixel jitter offset.
		/// [JP] 上のTAAジッター付きprojectionではなくnonJitterProjection
		///      を使う - ゲームプレイのスクリーン⇔ワールド変換は、
		///      描画専用のサブピクセルジッターオフセットではなく
		///      カメラ本来のprojectionを使うべきなため。
		ScreenSpace::SetCurrentView(view, nonJitterProjection, rect_);
	}
}
