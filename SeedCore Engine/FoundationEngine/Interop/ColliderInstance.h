#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/// [EN] Wireframe primitive for a collider. The values are the line shader's shape numbers, shared with ShapeKind.
	/// [JP] コライダーのワイヤーフレームの種類。値は線描画シェーダーの形の番号で、ShapeKind と共通。
	enum class ColliderKind :Uint32
	{
		Box = 0,
		Sphere = 1,
		Capsule = 2,
		Cylinder = 3,
		Rect = 7,
		Circle = 8,
	};

	/**
	* [EN]
	* One collider to draw as wireframe lines. It sits in FoundationEngine
	* so it can be built from the collider components; the renderer packs it
	* into its GPU buffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワイヤーフレームの線で描くコライダー1つ。コライダーのコンポーネントから
	* 作れるよう FoundationEngine に置く。レンダラーがこれを GPU 用のバッファに
	* 詰めて描く。
	*/
	struct ColliderDesc
	{
		/// [EN] Which wireframe to draw.
		/// [JP] 描く形の種類。
		ColliderKind kind_ = ColliderKind::Box;

		/// [EN] World position of the collider's center.
		/// [JP] コライダーの中心のワールド位置。
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] World rotation of the collider.
		/// [JP] コライダーのワールドでの回転。
		Quaternion rotation_ = Quaternion::Identity;

		/// [EN] Size of the collider; what each component means depends on kind_ (half extents for Box, radius for Sphere, radius and half height for Capsule and Cylinder, and so on).
		/// [JP] コライダーの大きさ。各成分の意味は kind_ で決まる（Box は半分の大きさ、Sphere は半径、Capsule と Cylinder は半径と高さの半分など）。
		Vector3 dimensions_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Line color.
		/// [JP] 線の色。
		Color color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	};
}
