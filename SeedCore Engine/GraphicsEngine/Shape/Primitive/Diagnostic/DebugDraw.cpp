#include <GraphicsEngine/Shape/Primitive/Diagnostic/DebugDraw.h>

namespace SeedCore
{
	/**
	* [EN]
	* Constructs the entry, recording every shape into renderInstance.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 窓口を構築する。形はすべて renderInstance に記録する。
	*/
	DebugDraw::DebugDraw(RenderInstance& renderInstance) :renderInstance_(renderInstance)
	{
		/// No Code
	}

	/**
	* [EN]
	* Draws a line from start to end.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* start から end へ線を描く。
	*/
	void DebugDraw::Line(const Vector3& start, const Vector3& end, const Color& color)
	{
#ifdef _DEBUG
		/// [EN] A segment starts at its position and runs along its dimensions, so the dimensions are the vector to the end.
		/// [JP] 線分は位置から始まり大きさの向きに伸びるので、大きさは終点までのベクトル。
		ShapeDesc shape{};
		shape.kind_ = ShapeKind::Segment;
		shape.style_ = ShapeStyle::Wireframe;
		shape.space_ = ShapeSpace::World;
		shape.scope_ = ShapeScope::Game;
		shape.position_ = start;
		shape.rotation_ = Quaternion::Identity;
		shape.dimensions_ = end - start;
		shape.color_ = color;
		renderInstance_.Add(shape);
#endif
	}

	/**
	* [EN]
	* Draws an arrow from start to end. headLength caps the head's length
	* in meters; 0 leaves the head at its fixed fraction of the arrow's
	* length.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* start から end へ矢印を描く。headLength は矢じりの長さの上限
	* （メートル）。0 なら矢じりは矢印の長さに対する決まった割合のまま。
	*/
	void DebugDraw::Arrow(const Vector3& start, const Vector3& end, const Color& color, Float headLength)
	{
#ifdef _DEBUG
		/// [EN] Placed the same way as a segment: from the start, along the vector to the end.
		/// [JP] 線分と同じ置き方。始点から、終点までのベクトルの向きに伸ばす。
		ShapeDesc shape{};
		shape.kind_ = ShapeKind::Arrow;
		shape.style_ = ShapeStyle::Wireframe;
		shape.space_ = ShapeSpace::World;
		shape.scope_ = ShapeScope::Game;
		shape.position_ = start;
		shape.rotation_ = Quaternion::Identity;
		shape.dimensions_ = end - start;
		shape.color_ = color;
		shape.headLength_ = headLength;
		renderInstance_.Add(shape);
#endif
	}

	/**
	* [EN]
	* Draws a box of the given full size, centered on center.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* center を中心に、size の大きさ（全体の幅・高さ・奥行き）の箱を描く。
	*/
	void DebugDraw::Box(const Vector3& center, const Quaternion& rotation, const Vector3& size, const Color& color)
	{
#ifdef _DEBUG
		/// [EN] The unit box spans -1 to 1, so the dimensions are half the size.
		/// [JP] 単位の箱は -1 から 1 なので、大きさはサイズの半分。
		ShapeDesc shape{};
		shape.kind_ = ShapeKind::Box;
		shape.style_ = ShapeStyle::Wireframe;
		shape.space_ = ShapeSpace::World;
		shape.scope_ = ShapeScope::Game;
		shape.position_ = center;
		shape.rotation_ = rotation;
		shape.dimensions_ = Vector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
		shape.color_ = color;
		renderInstance_.Add(shape);
#endif
	}

	/**
	* [EN]
	* Draws a sphere centered on center.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* center を中心に球を描く。
	*/
	void DebugDraw::Sphere(const Vector3& center, Float radius, const Color& color)
	{
#ifdef _DEBUG
		/// [EN] A sphere reads only its radius, from the dimensions' X.
		/// [JP] 球が使うのは大きさの X に入れた半径だけ。
		ShapeDesc shape{};
		shape.kind_ = ShapeKind::Sphere;
		shape.style_ = ShapeStyle::Wireframe;
		shape.space_ = ShapeSpace::World;
		shape.scope_ = ShapeScope::Game;
		shape.position_ = center;
		shape.rotation_ = Quaternion::Identity;
		shape.dimensions_ = Vector3(radius, 0.0f, 0.0f);
		shape.color_ = color;
		renderInstance_.Add(shape);
#endif
	}

	/**
	* [EN]
	* Draws a capsule centered on center, running along its local Y. height
	* is the cylinder part between the two caps, as in CapsuleCollider.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* center を中心に、ローカルの Y 方向に伸びるカプセルを描く。height は
	* CapsuleCollider と同じく、両端の半球にはさまれた円柱部分の高さ。
	*/
	void DebugDraw::Capsule(const Vector3& center, const Quaternion& rotation, Float radius, Float height, const Color& color)
	{
#ifdef _DEBUG
		/// [EN] The dimensions are the radius and half the cylinder part's height.
		/// [JP] 大きさは、半径と、円柱部分の高さの半分。
		ShapeDesc shape{};
		shape.kind_ = ShapeKind::Capsule;
		shape.style_ = ShapeStyle::Wireframe;
		shape.space_ = ShapeSpace::World;
		shape.scope_ = ShapeScope::Game;
		shape.position_ = center;
		shape.rotation_ = rotation;
		shape.dimensions_ = Vector3(radius, height * 0.5f, 0.0f);
		shape.color_ = color;
		renderInstance_.Add(shape);
#endif
	}

	/**
	* [EN]
	* Draws a cylinder centered on center, running along its local Y.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* center を中心に、ローカルの Y 方向に伸びる円柱を描く。
	*/
	void DebugDraw::Cylinder(const Vector3& center, const Quaternion& rotation, Float radius, Float height, const Color& color)
	{
#ifdef _DEBUG
		/// [EN] The dimensions are the radius and half the height.
		/// [JP] 大きさは、半径と高さの半分。
		ShapeDesc shape{};
		shape.kind_ = ShapeKind::Cylinder;
		shape.style_ = ShapeStyle::Wireframe;
		shape.space_ = ShapeSpace::World;
		shape.scope_ = ShapeScope::Game;
		shape.position_ = center;
		shape.rotation_ = rotation;
		shape.dimensions_ = Vector3(radius, height * 0.5f, 0.0f);
		shape.color_ = color;
		renderInstance_.Add(shape);
#endif
	}
}
