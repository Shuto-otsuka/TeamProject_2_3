#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	/**
	* [EN]
	* Primitive of a shape that is not a collider (light ranges, camera
	* frustums, bones, and shape components). The values are the line
	* shader's shape numbers, shared with ColliderKind.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コライダー以外の形（ライトの範囲、カメラの視錐台、ボーン、形の
	* コンポーネントなど）の種類。値は線描画シェーダーの形の番号で、
	* ColliderKind と共通。
	*/
	enum class ShapeKind :Uint32
	{
		Box = 0,
		Sphere = 1,
		Capsule = 2,
		Cylinder = 3,
		Cone = 4,
		Ramp = 5,
		Torus = 6,
		Rect = 7,
		Circle = 8,
		Plane = 9,
		Disc = 10,
		Segment = 11,
		Arrow = 12,
	};

	/**
	* [EN]
	* How a shape is drawn; the renderer sorts shapes into one batch per
	* style.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 形の描き方。レンダラーは描き方ごとのバッチに振り分ける。
	*/
	enum class ShapeStyle :Uint32
	{
		/// [EN] Lines (debug display such as light ranges, camera frustums and physics queries).
		/// [JP] 線（ライトの範囲、カメラの視錐台、物理クエリなどのデバッグ表示）。
		Wireframe,

		/// [EN] Filled surfaces (shape components such as a box shape).
		/// [JP] 面（箱の形などの、形のコンポーネント）。
		Solid,
	};

	/**
	* [EN]
	* Which space a shape is placed in, and so whether it is drawn in the
	* 3D views or on the canvas.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 形を置く空間。3D のビューに描くか、Canvas に描くかもこれで決まる。
	*/
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
	* Which 3D views draw a world shape. Canvas shapes are drawn on the
	* canvas whatever this says.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワールドの形をどの 3D ビューに描くか。Canvas の形は、これに関係
	* なく Canvas に描く。
	*/
	enum class ShapeScope :Uint32
	{
		/// [EN] Only the editor view (editor-only display such as camera frustums and light ranges).
		/// [JP] エディタービューだけ（カメラの視錐台やライトの範囲などの、エディター専用の表示）。
		Editor,

		/// [EN] The game view as well as the editor view (physics queries, shape components).
		/// [JP] エディタービューに加えてゲームビューにも描く（物理クエリ、形のコンポーネント）。
		Game,
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

		/// [EN] Space the shape is placed in.
		/// [JP] 形を置く空間。
		ShapeSpace space_ = ShapeSpace::World;

		/// [EN] Which 3D views draw the shape.
		/// [JP] 形を描く 3D ビュー。
		ShapeScope scope_ = ShapeScope::Editor;

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

		/// [EN] Arrow only: upper bound of the head's length, in the same units as dimensions_ (meters in the world, pixels on the canvas); 0 leaves the head at its fixed fraction of the arrow's length.
		/// [JP] Arrow のときだけ使う、矢じりの長さの上限。単位は dimensions_ と同じ（ワールドはメートル、Canvas はピクセル）。0 なら矢じりは矢印の長さに対する決まった割合のまま。
		Float headLength_ = 0.0f;

		/// [EN] Wireframe only: on-screen line width in pixels; silhouette lines are drawn 1.5 times this.
		/// [JP] Wireframe のときだけ使う、画面上の線の太さ（ピクセル）。輪郭線はこの 1.5 倍で描く。
		Float lineWidth_ = 3.0f;

		/// [EN] Solid only: asset ID of the texture laid over the surfaces; 0 draws the color alone. The renderer resolves it to a bindless index.
		/// [JP] Solid のときだけ使う、面に張るテクスチャのアセット ID。0 なら色だけで塗る。レンダラーが bindless の番号に直す。
		Uint32 textureID_ = 0;

		/// [EN] Solid only: how many times the texture repeats across each surface.
		/// [JP] Solid のときだけ使う、各面にテクスチャを並べる回数。
		Vector2 uvScale_ = { 1.0f, 1.0f };

		/// [EN] Solid only: where the texture starts on each surface.
		/// [JP] Solid のときだけ使う、各面でのテクスチャの開始位置。
		Vector2 uvOffset_ = { 0.0f, 0.0f };

		/// [EN] Solid only: whether the back faces are drawn as well.
		/// [JP] Solid のときだけ使う、裏面も描くか。
		Bool doubleSided_ = false;
	};
}
