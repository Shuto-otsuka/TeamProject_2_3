#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Math/Halton.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <GraphicsEngine/System/SceneSystem.h>
#include <GraphicsEngine/Camera/EditorCamera.h>
#include <GraphicsEngine/Camera/EditorCameraController.h>

namespace SeedCore
{
	class World;
	class GameTimer;

	/**
	* [EN]
	* Which camera the game view renders from.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームビューがどのカメラで描画するか。
	*/
	enum class CameraMode
	{
		/// [EN] The Editor's free-fly camera.
		/// [JP] Editor のフリーカメラ。
		Free,

		/// [EN] The game's own active Camera.
		/// [JP] ゲーム自身のアクティブな Camera。
		User,
	};

	/**
	* [EN]
	* Resolves the camera the game view renders from each frame and
	* publishes it as the scene constant buffer. In User mode that is the
	* game's own active Camera (blended through CameraBrain); in Free mode
	* it is the Editor's free-fly camera.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームビューが毎フレーム描画に使うカメラを解決し、シーン定数バッファ
	* として公開する。User モードではゲーム自身のアクティブな Camera
	* (CameraBrain によるブレンド込み)、Free モードでは Editor の
	* フリーカメラになる。
	*/
	class SEEDCORE_API CameraSystem
	{
	public:
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
		void Update(World& world, GameTimer& timer, Float renderWidth, Float renderHeight);

		/**
		* [EN]
		* Feeds the Editor's mouse/keyboard input to the free-fly camera.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Editor のマウス/キーボード入力をフリーカメラへ渡す。
		*/
		void Navigate(Float deltaTime);

	public:
		/**
		* [EN]
		* Selects which camera the game view renders from.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲームビューがどのカメラで描画するかを選ぶ。
		*/
		void Mode(CameraMode mode);

		/**
		* [EN]
		* Returns which camera the game view renders from.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲームビューがどのカメラで描画しているかを返す。
		*/
		CameraMode Mode()const;

	public:
		/**
		* [EN]
		* Sets where the game image is displayed on the desktop, in desktop
		* pixels (x = left, y = top, z = width, w = height). Called once per
		* frame by whoever presents the game image (the Editor's
		* GameWindowPanel, the Runtime's Engine); Update() forwards it to
		* ScreenSpace so the camera frustum maps onto exactly this rectangle.
		* A degenerate size is ignored.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ゲーム画像がデスクトップ上のどこに表示されているかを、
		* デスクトップのピクセル座標(x = 左、y = 上、z = 幅、w = 高さ)で
		* 設定する。ゲーム画像を表示する側(Editor の GameWindowPanel、
		* Runtime の Engine)が毎フレーム呼び出し、Update() がそれを
		* ScreenSpace へ渡すので、カメラの視錐台がちょうどこの矩形に対応する。
		* 大きさが0以下のものは無視する。
		*/
		void Rect(const Vector4& rect);

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
		Vector4 Rect()const;

	public:
		/**
		* [EN]
		* Whether an active Camera was found by the last Update().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直前の Update() でアクティブな Camera が見つかったかどうか。
		*/
		Bool ActiveCamera()const;

		/**
		* [EN]
		* Returns the scene constant buffer built by the last Update().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直前の Update() で作られたシーン定数バッファを返す。
		*/
		const SceneConstantBuffer& GetSceneConstantBuffer()const;

	private:
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
		void SyncCameraBrains(World& world, Float aspectRatio);

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
		void UpdateFreeCamera(World& world, GameTimer& timer, Float renderWidth, Float renderHeight);

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
		void UpdateUserCamera(World& world, GameTimer& timer, Float renderWidth, Float renderHeight);

	private:
		/// [EN] Which camera the game view renders from.
		/// [JP] ゲームビューがどのカメラで描画するか。
		CameraMode mode_ = CameraMode::User;

		/// [EN] Where the game image is displayed on the desktop (x = left, y = top, z = width, w = height).
		/// [JP] ゲーム画像がデスクトップ上で表示されている位置(x = 左、y = 上、z = 幅、w = 高さ)。
		Vector4 rect_ = Vector4(0.0f, 0.0f, 1.0f, 1.0f);

		/// [EN] Whether the last Update() found an active Camera.
		/// [JP] 直前の Update() でアクティブな Camera が見つかったかどうか。
		Bool hasActiveCamera_ = false;

		/// [EN] Scene constants built by the last Update().
		/// [JP] 直前の Update() で作られたシーン定数。
		SceneConstantBuffer sceneConstantBuffer_{};

		/// [EN] Last frame's jittered view-projection, for motion vectors.
		/// [JP] 前フレームのジッター付き view-projection。モーションベクトル用。
		Matrix previousViewProjection_ = Matrix::Identity;

		/// [EN] Last frame's view-projection without jitter, for motion vectors.
		/// [JP] 前フレームのジッター無し view-projection。モーションベクトル用。
		Matrix previousNonJitterViewProjection_ = Matrix::Identity;

		/// [EN] This frame's TAA sub-pixel jitter, in pixels.
		/// [JP] 今フレームの TAA サブピクセルジッター。ピクセル単位。
		Vector2 jitter_ = Vector2(0.5f, 0.5f);

		/// [EN] Index into the Halton sequence that drives jitter_.
		/// [JP] jitter_ を決める Halton 数列の添字。
		Uint32 frameIndex_ = 0;

		/// [EN] CameraBrain currently driving the User camera.
		/// [JP] 現在 User カメラを動かしている CameraBrain。
		EntityID activeBrain_;

		/// [EN] Eye position resolved last frame, the start point of a new blend.
		/// [JP] 前フレームで解決した視点位置。新しいブレンドの開始点。
		Vector3 lastEye_ = Vector3::Zero;

		/// [EN] Orientation resolved last frame, the start point of a new blend.
		/// [JP] 前フレームで解決した向き。新しいブレンドの開始点。
		Quaternion lastOrientation_ = Quaternion::Identity;

		/// [EN] Eye position the current blend starts from.
		/// [JP] 現在のブレンドが始まった視点位置。
		Vector3 blendFromEye_ = Vector3::Zero;

		/// [EN] Orientation the current blend starts from.
		/// [JP] 現在のブレンドが始まった向き。
		Quaternion blendFromOrientation_ = Quaternion::Identity;

		/// [EN] Seconds elapsed in the current blend.
		/// [JP] 現在のブレンドの経過秒数。
		Float blendElapsed_ = 0.0f;

		/// [EN] Total seconds of the current blend; 0 means no blend in progress.
		/// [JP] 現在のブレンドの総秒数。0 ならブレンドしていない。
		Float blendDuration_ = 0.0f;

		/// [EN] The Editor's free-fly camera used in Free mode.
		/// [JP] Free モードで使う Editor のフリーカメラ。
		EditorCamera freeCamera_;

		/// [EN] Turns Editor input into free-fly camera movement.
		/// [JP] Editor の入力をフリーカメラの動きに変える。
		EditorCameraController freeCameraController_;
	};
}
