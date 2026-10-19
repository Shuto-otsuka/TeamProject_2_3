#pragma once
#include <FoundationEngine/Math/Ray.h>

namespace SeedCore
{
	/**
	* [EN]
	* Converts between screen-pixel coordinates and world space, always
	* against the game's own active Camera (the one CameraSystem
	* computes each frame from the ECS's Camera component) - never the
	* Editor's own free-fly tool camera. Screen pixels are on the
	* 1920x1080 game screen (origin at its top-left), the same space as
	* Input::MousePoint() and the canvas. Callers (SeedScript/UserProject
	* code) never pass in a view/projection: CameraSystem pushes the
	* camera via SetCurrentView(), and the game screen does not depend on
	* where the game image is displayed, so ScreenToWorld()/WorldToScreen()
	* behave identically in the Editor's ゲームビュー and the Runtime
	* window - there is no "which view" to specify.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スクリーンのピクセル座標とワールド空間を変換する。常にゲーム自身の
	* アクティブな Camera(CameraSystem が毎フレーム ECS の Camera
	* コンポーネントから計算するもの)を基準にする - Editor 自身の
	* フリーカメラ(ツールカメラ)は対象にしない。スクリーンピクセルは
	* 1920x1080 のゲーム画面上の座標(原点はその左上)で、
	* Input::MousePoint() や Canvas と同じ座標系。呼び出し側
	* (SeedScript/UserProject のコード)は view/projection を一切渡さない -
	* CameraSystem が SetCurrentView() でカメラを渡し、ゲーム画面は
	* ゲーム画像の表示位置に左右されないので、ScreenToWorld()/
	* WorldToScreen() は Editor のゲームビューと Runtime のウィンドウの
	* どちらから呼んでも同じ挙動になる - 「どちらのビューか」を指定する
	* 必要が無い。
	*/
	class CameraSystem;

	class SEEDCORE_API ScreenSpace
	{
		friend class CameraSystem;

	public:
		/**
		* [EN]
		* Converts pixelPosition (1920x1080 game screen pixels, the same
		* space as Input::MousePoint()) into a world-space Ray from the
		* current camera's near plane through pixelPosition, suitable for
		* passing straight into Physics::Raycast()/Spherecast().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* pixelPosition(1920x1080 のゲーム画面のピクセル座標。
		* Input::MousePoint() と同じ座標系)を、現在のカメラの近平面から pixelPosition を通る
		* ワールド空間の Ray へ変換する。
		* Physics::Raycast()/Spherecast() にそのまま渡せる。
		*/
		static Ray ScreenToWorld(const Vector2& pixelPosition);

		/**
		* [EN]
		* Converts worldPosition into 1920x1080 game screen pixels (the
		* same space as Input::MousePoint()) under the current camera.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* worldPosition を、現在のカメラでの 1920x1080 のゲーム画面のピクセル座標
		* (Input::MousePoint() と同じ座標系)へ変換する。
		*/
		static Vector2 WorldToScreen(const Vector3& worldPosition);

	private:
		/**
		* [EN]
		* Called by CameraSystem once per frame (after computing the
		* game's active Camera's view/projection) to publish the values
		* ScreenToWorld()/WorldToScreen() use. The whole camera frustum
		* maps onto the 1920x1080 game screen, independent of the internal
		* render resolution. Private + friended to CameraSystem rather than
		* merely documented as internal-only, so gameplay code can't
		* accidentally feed it an arbitrary view.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CameraSystem が毎フレーム(ゲームのアクティブな Camera の
		* view/projection を計算した後に)呼び出し、
		* ScreenToWorld()/WorldToScreen() が使う値を公開する。カメラの視錐台
		* 全体が 1920x1080 のゲーム画面に対応し、内部描画解像度には左右されない。
		* 「内部専用」とドキュメントで言うだけでなく、private化して
		* CameraSystem だけを friend にすることで、ゲームプレイコードが
		* 誤って任意の view を流し込めないようにする。
		*/
		static void SetCurrentView(const Matrix& view, const Matrix& projection);

	private:
		/// [EN] View matrix of the game's active camera.
		/// [JP] ゲームのアクティブなカメラのビュー行列。
		static Matrix view_;

		/// [EN] Projection matrix of the game's active camera, without TAA jitter.
		/// [JP] ゲームのアクティブなカメラの、TAAジッターを含まない射影行列。
		static Matrix projection_;
	};
}
