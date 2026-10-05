#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Math/Ray.h>

namespace SeedCore
{
	/**
	* [EN]
	* Converts between screen-pixel coordinates and world space, always
	* against the game's own active Camera (the one CameraSystem
	* computes each frame from the ECS's Camera component) - never the
	* Editor's own free-fly tool camera. Screen pixels are desktop
	* coordinates (origin at the primary monitor's top-left), the same
	* space as Input::MousePoint(). Callers (SeedScript/UserProject code)
	* never pass in a view/projection/display rectangle: CameraSystem
	* pushes the camera together with where the game image sits on the
	* desktop (set on it by the Editor's ゲームビュー or the Runtime
	* window) via SetCurrentView(), so ScreenToWorld()/WorldToScreen()
	* behave identically in both - there is no "which view" to specify.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* スクリーンのピクセル座標とワールド空間を変換する。常にゲーム自身の
	* アクティブな Camera(CameraSystem が毎フレーム ECS の Camera
	* コンポーネントから計算するもの)を基準にする - Editor 自身の
	* フリーカメラ(ツールカメラ)は対象にしない。スクリーンピクセルは
	* デスクトップ座標(原点はプライマリモニタの左上)で、
	* Input::MousePoint() と同じ座標系。呼び出し側
	* (SeedScript/UserProject のコード)は view/projection/表示矩形を
	* 一切渡さない - CameraSystem が SetCurrentView() で、カメラと一緒に
	* ゲーム画像がデスクトップ上のどこに表示されているか(Editor の
	* ゲームビュー または Runtime のウィンドウが CameraSystem に設定する)
	* を反映させるため、ScreenToWorld()/WorldToScreen() はどちらから
	* 呼んでも同じ挙動になる - 「どちらのビューか」を指定する必要が無い。
	*/
	class CameraSystem;

	class SEEDCORE_API ScreenSpace
	{
		friend class CameraSystem;

	public:
		/**
		* [EN]
		* Converts pixelPosition (desktop pixels, the same space as
		* Input::MousePoint()) into a world-space Ray from the current
		* camera's near plane through pixelPosition, suitable for passing
		* straight into Physics::Raycast()/Spherecast().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* pixelPosition(デスクトップのピクセル座標。Input::MousePoint() と
		* 同じ座標系)を、現在のカメラの近平面から pixelPosition を通る
		* ワールド空間の Ray へ変換する。
		* Physics::Raycast()/Spherecast() にそのまま渡せる。
		*/
		static Ray ScreenToWorld(const Vector2& pixelPosition);

		/**
		* [EN]
		* Converts worldPosition into desktop pixels (the same space as
		* Input::MousePoint()) under the current camera.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* worldPosition を、現在のカメラでのデスクトップのピクセル座標
		* (Input::MousePoint() と同じ座標系)へ変換する。
		*/
		static Vector2 WorldToScreen(const Vector3& worldPosition);

	private:
		/**
		* [EN]
		* Called by CameraSystem once per frame (after computing the
		* game's active Camera's view/projection) to publish the values
		* ScreenToWorld()/WorldToScreen() use. rect is where the game image
		* is displayed on the desktop (x = left, y = top, z = width,
		* w = height, in desktop pixels); the whole camera frustum maps onto
		* it, independent of the internal render resolution. Private +
		* friended to CameraSystem rather than merely documented as
		* internal-only, so gameplay code can't accidentally feed it an
		* arbitrary view.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* CameraSystem が毎フレーム(ゲームのアクティブな Camera の
		* view/projection を計算した後に)呼び出し、
		* ScreenToWorld()/WorldToScreen() が使う値を公開する。rect は
		* ゲーム画像がデスクトップ上で表示されている位置(x = 左、y = 上、
		* z = 幅、w = 高さ。デスクトップのピクセル座標)で、カメラの視錐台
		* 全体がこれに対応し、内部描画解像度には左右されない。
		* 「内部専用」とドキュメントで言うだけでなく、private化して
		* CameraSystem だけを friend にすることで、ゲームプレイコードが
		* 誤って任意の view を流し込めないようにする。
		*/
		static void SetCurrentView(const Matrix& view, const Matrix& projection, const Vector4& rect);

	private:
		/// [EN] View matrix of the game's active camera.
		/// [JP] ゲームのアクティブなカメラのビュー行列。
		static Matrix view_;

		/// [EN] Projection matrix of the game's active camera, without TAA jitter.
		/// [JP] ゲームのアクティブなカメラの、TAAジッターを含まない射影行列。
		static Matrix projection_;

		/// [EN] Where the game image is displayed on the desktop (x = left, y = top, z = width, w = height).
		/// [JP] ゲーム画像がデスクトップ上で表示されている位置(x = 左、y = 上、z = 幅、w = 高さ)。
		static Vector4 rect_;
	};
}
