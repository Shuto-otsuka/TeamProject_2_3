#include <GraphicsEngine/Camera/ScreenSpace.h>

namespace SeedCore
{
	Matrix ScreenSpace::view_ = Matrix::Identity;
	Matrix ScreenSpace::projection_ = Matrix::Identity;
	Vector4 ScreenSpace::rect_ = Vector4(0.0f, 0.0f, 1.0f, 1.0f);

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
	Ray ScreenSpace::ScreenToWorld(const Vector2& pixelPosition)
	{
		/// [EN] Desktop pixel -> position inside the displayed game image, normalized to [0, 1].
		/// [JP] デスクトップのピクセル座標→表示中のゲーム画像内の位置を[0, 1]に正規化したもの。
		Float normalizedX = (pixelPosition.x - rect_.x) / rect_.z;
		Float normalizedY = (pixelPosition.y - rect_.y) / rect_.w;

		/// [EN] Normalized -> NDC: X maps [0, 1] to [-1, 1], Y maps [0, 1]
		///      to [1, -1] (screen Y grows downward, NDC Y grows upward).
		/// [JP] 正規化座標→NDC: Xは[0, 1]を[-1, 1]へ、Yは[0, 1]を[1, -1]へ
		///      写す(スクリーンYは下向き、NDCのYは上向きに増えるため)。
		Float ndcX = normalizedX * 2.0f - 1.0f;
		Float ndcY = 1.0f - normalizedY * 2.0f;

		Matrix inverseViewProjection = (view_ * projection_).Invert();

		/// [EN] Vector3::Transform divides by w (unlike Vector4::Transform),
		///      so these already come back as ordinary world-space points -
		///      one on the near plane, one on the far plane, both under
		///      the same (ndcX, ndcY) screen column.
		/// [JP] Vector3::Transform は(Vector4::Transformと違って)wで
		///      除算するため、これらは既に通常のワールド空間の点として
		///      返ってくる - 同じ(ndcX, ndcY)のスクリーン列上の、近平面上の
		///      点と遠平面上の点。
		Vector3 nearPoint = Vector3::Transform(Vector3(ndcX, ndcY, 1.0f), inverseViewProjection);
		
		Vector3 cameraPosition = view_.Invert().Translation();

		Ray ray;
		ray.origin_ = nearPoint;
		ray.direction_ = nearPoint - cameraPosition;
		if (ray.direction_.LengthSquared() > 0.0f)
		{
			ray.direction_.Normalize();
		}
		return ray;
	}

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
	Vector2 ScreenSpace::WorldToScreen(const Vector3& worldPosition)
	{
		Vector3 clipPosition = Vector3::Transform(worldPosition, view_ * projection_);

		/// [EN] NDC -> position inside the displayed game image, then offset by where that image sits on the desktop.
		/// [JP] NDC→表示中のゲーム画像内の位置に変換し、その画像のデスクトップ上の位置を足す。
		Float pixelX = rect_.x + (clipPosition.x * 0.5f + 0.5f) * rect_.z;
		Float pixelY = rect_.y + (1.0f - (clipPosition.y * 0.5f + 0.5f)) * rect_.w;
		return Vector2(pixelX, pixelY);
	}

	/**
	* [EN]
	* Called by CameraSystem once per frame (after computing the game's
	* active Camera's view/projection) to publish the values
	* ScreenToWorld()/WorldToScreen() use, including where the game image
	* is displayed on the desktop - not meant to be called from gameplay
	* code.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* CameraSystem が毎フレーム(ゲームのアクティブな Camera の
	* view/projection を計算した後に)呼び出し、
	* ScreenToWorld()/WorldToScreen() が使う値(ゲーム画像のデスクトップ
	* 上の表示位置を含む)を公開する - ゲームプレイコードから呼ぶことは
	* 想定していない。
	*/
	void ScreenSpace::SetCurrentView(const Matrix& view, const Matrix& projection, const Vector4& rect)
	{
		view_ = view;
		projection_ = projection;
		rect_ = rect;
	}
}
