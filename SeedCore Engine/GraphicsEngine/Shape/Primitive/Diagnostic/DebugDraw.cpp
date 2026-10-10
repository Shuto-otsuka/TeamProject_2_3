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
	* Draws a filled arrow from start to end: a cylinder shaft and a cone
	* head. headLength caps the head's length in meters; 0 leaves the head
	* at its fixed fraction of the arrow's length. The head's radius and
	* the shaft's radius follow the head's length.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* start から end へ、面で塗った矢印を描く。胴体は円柱、矢じりは円錐。
	* headLength は矢じりの長さの上限（メートル）。0 なら矢じりは矢印の
	* 長さに対する決まった割合のまま。矢じりと胴体の半径は、矢じりの
	* 長さに合わせて決まる。
	*/
	void DebugDraw::Arrow(const Vector3& start, const Vector3& end, const Color& color, Float headLength)
	{
#ifdef _DEBUG
		/// [EN] An arrow with no length has no direction to point in, so nothing is drawn.
		/// [JP] 長さの無い矢印は向きを持たないので、何も描かない。
		Vector3 direction = end - start;
		Float length = direction.Length();
		if (length < 1e-5f)
		{
			return;
		}
		direction /= length;

		/// [EN] The same proportions as the wireframe arrow: the head is a fifth of the length, kept within headLength when one is given, and its base radius is 0.4 of its length. The shaft is a third as thick as the head's base.
		/// [JP] ワイヤーフレームの矢印と同じ比率。矢じりは全体の長さの 5 分の 1 で、headLength があればそれ以下に収め、底面の半径は矢じりの長さの 0.4 倍。胴体の太さは矢じりの底面の 3 分の 1。
		Float arrowHeadLength = length * 0.2f;
		if (headLength > 0.0f)
		{
			arrowHeadLength = Min(arrowHeadLength, headLength);
		}
		Float headRadius = arrowHeadLength * 0.4f;
		Float shaftRadius = headRadius / 3.0f;
		Float shaftLength = length - arrowHeadLength;

		/// [EN] The unit cylinder and cone run along their local Y, so both are turned from +Y onto the arrow's direction.
		/// [JP] 単位の円柱と円錐はローカルの Y 方向に伸びるので、どちらも +Y から矢印の向きへ回す。
		Quaternion rotation = Quaternion::FromToRotation(Vector3::UnitY, direction);

		/// [EN] The shaft, centered halfway between the start and the head's base.
		/// [JP] 胴体。始点と矢じりの底面の真ん中に置く。
		ShapeDesc shaft{};
		shaft.kind_ = ShapeKind::Cylinder;
		shaft.style_ = ShapeStyle::Solid;
		shaft.space_ = ShapeSpace::World;
		shaft.scope_ = ShapeScope::Game;
		shaft.position_ = start + direction * (shaftLength * 0.5f);
		shaft.rotation_ = rotation;
		shaft.dimensions_ = Vector3(shaftRadius, shaftLength * 0.5f, 0.0f);
		shaft.color_ = color;
		renderInstance_.Add(shaft);

		/// [EN] The head, centered halfway along its own length so its base meets the shaft and its apex lands on the end.
		/// [JP] 矢じり。自分の長さの真ん中に置くので、底面が胴体とつながり、頂点が終点に来る。
		ShapeDesc head{};
		head.kind_ = ShapeKind::Cone;
		head.style_ = ShapeStyle::Solid;
		head.space_ = ShapeSpace::World;
		head.scope_ = ShapeScope::Game;
		head.position_ = start + direction * (shaftLength + arrowHeadLength * 0.5f);
		head.rotation_ = rotation;
		head.dimensions_ = Vector3(headRadius, arrowHeadLength * 0.5f, 0.0f);
		head.color_ = color;
		renderInstance_.Add(head);
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
