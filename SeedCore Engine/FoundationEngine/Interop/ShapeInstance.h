#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/// [EN] Primitive of a shape that is not a collider (light ranges, camera frustums, bones, and shape components). The values are the line shader's shape numbers, shared with ColliderKind.
	/// [JP] コライダー以外の形（ライトの範囲、カメラの視錐台、ボーン、形のコンポーネントなど）の種類。値は線描画シェーダーの形の番号で、ColliderKind と共通。
	enum class ShapeKind :Uint32
	{
		Box = 0,
		Sphere = 1,
		Capsule = 2,
		Cylinder = 3,
		Rect = 4,
		Circle = 5,
		Cone = 6,
		Segment = 7,
		Arrow = 8,
	};

	/// [EN] How a shape is drawn; the renderer sorts shapes into one batch per style.
	/// [JP] 形の描き方。レンダラーは描き方ごとのバッチに振り分ける。
	enum class ShapeStyle :Uint32
	{
		/// [EN] Lines, drawn only in the editor view (debug display such as light ranges and camera frustums).
		/// [JP] 線。エディタービューにだけ描く（ライトの範囲やカメラの視錐台などのデバッグ表示）。
		Wireframe,

		/// [EN] Filled surfaces, drawn in both the game and editor views (shape components such as a box shape).
		/// [JP] 面。ゲームとエディターの両方のビューに描く（箱の形などの、形のコンポーネント）。
		Solid,
	};

	/// [EN] Which space a shape is placed in, and so which view draws it.
	/// [JP] 形を置く空間。どのビューに描くかもこれで決まる。
	enum class ShapeSpace :Uint32
	{
		/// [EN] The 3D world, in meters.
		/// [JP] 3D のワールド。単位はメートル。
		World,

		/// [EN] The canvas, in the canvas view's render coordinates (the same placement as the 2D colliders).
		/// [JP] Canvas。Canvas ビューの描画座標（2D のコライダーと同じ置き方）。
		Canvas,
	};

	/**
	* [EN]
	* One shape to draw, either as wireframe lines (light ranges, camera
	* frustums and so on) or as filled surfaces (shape components). Any module
	* can build these, since it sits in FoundationEngine; the renderer packs
	* them into its GPU buffers.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 描く形1つ。線で描くもの（ライトの範囲、カメラの視錐台など）と、面で
	* 描くもの（形のコンポーネント）がある。FoundationEngine にあるので、
	* どのモジュールからでも作れる。レンダラーがこれを GPU 用のバッファに
	* 詰めて描く。
	*/
	struct ShapeDesc
	{
		/// [EN] Which shape to draw.
		/// [JP] 描く形の種類。
		ShapeKind kind_ = ShapeKind::Segment;

		/// [EN] Whether the shape is drawn as lines or as filled surfaces.
		/// [JP] 線で描くか、面で描くか。
		ShapeStyle style_ = ShapeStyle::Wireframe;

		/// [EN] World position of the shape's origin.
		/// [JP] 形の原点のワールド位置。
		Vector3 position_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] World rotation of the shape.
		/// [JP] 形のワールドでの回転。
		Quaternion rotation_ = Quaternion::Identity;

		/// [EN] Size of the shape; what each component means depends on kind_ (half extents for Box, radius for Sphere, the vector to the end for Segment and Arrow, and so on).
		/// [JP] 形の大きさ。各成分の意味は kind_ で決まる（Box は半分の大きさ、Sphere は半径、Segment と Arrow は終点までのベクトルなど）。
		Vector3 dimensions_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Color of the lines or surfaces.
		/// [JP] 線または面の色。
		Color color_ = { 1.0f, 1.0f, 1.0f, 1.0f };

		/// [EN] Space the shape is placed in; last so that shapes listing only the fields above stay in the world.
		/// [JP] 形を置く空間。上のメンバーだけを並べて作った形はワールドに置かれるよう、最後に置く。
		ShapeSpace space_ = ShapeSpace::World;

		/// [EN] Arrow only: upper bound of the head's length, in the same units as dimensions_ (meters in the world, pixels on the canvas); 0 leaves the head at its fixed fraction of the arrow's length.
		/// [JP] Arrow のときだけ使う、矢じりの長さの上限。単位は dimensions_ と同じ（ワールドはメートル、Canvas はピクセル）。0 なら矢じりは矢印の長さに対する決まった割合のまま。
		Float headLength_ = 0.0f;
	};
}
