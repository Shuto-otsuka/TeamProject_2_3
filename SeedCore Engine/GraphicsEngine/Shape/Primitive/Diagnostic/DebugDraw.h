#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Interop/RenderInstance.h>

namespace SeedCore
{
	/**
	* [EN]
	* Script-facing entry for drawing wireframe debug shapes in the world.
	* Each call records one shape into the World's RenderInstance, drawn in
	* both the editor and game views for one frame. Calls are recorded in
	* debug builds only and do nothing in release builds.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワールドにワイヤーフレームのデバッグ用の形を描く、スクリプト向けの
	* 窓口。呼ぶたびに World の RenderInstance へ形を1つ記録し、エディター
	* ビューとゲームビューの両方に1フレームだけ描かれる。記録するのは
	* Debug ビルドだけで、Release ビルドでは何もしない。
	*/
	class SEEDCORE_API DebugDraw
	{
	public:
		/**
		* [EN]
		* Constructs the entry, recording every shape into renderInstance.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 窓口を構築する。形はすべて renderInstance に記録する。
		*/
		DebugDraw(RenderInstance& renderInstance);

		/**
		* [EN]
		* Destroys the entry.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 窓口を破棄する。
		*/
		~DebugDraw() = default;

	public:
		/**
		* [EN]
		* Draws a line from start to end.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* start から end へ線を描く。
		*/
		void Line(const Vector3& start, const Vector3& end, const Color& color);

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
		void Arrow(const Vector3& start, const Vector3& end, const Color& color, Float headLength = 0.0f);

		/**
		* [EN]
		* Draws a box of the given full size, centered on center.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* center を中心に、size の大きさ（全体の幅・高さ・奥行き）の箱を描く。
		*/
		void Box(const Vector3& center, const Quaternion& rotation, const Vector3& size, const Color& color);

		/**
		* [EN]
		* Draws a sphere centered on center.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* center を中心に球を描く。
		*/
		void Sphere(const Vector3& center, Float radius, const Color& color);

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
		void Capsule(const Vector3& center, const Quaternion& rotation, Float radius, Float height, const Color& color);

		/**
		* [EN]
		* Draws a cylinder centered on center, running along its local Y.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* center を中心に、ローカルの Y 方向に伸びる円柱を描く。
		*/
		void Cylinder(const Vector3& center, const Quaternion& rotation, Float radius, Float height, const Color& color);

	private:
		/// [EN] Record the shapes are added to, owned by World.
		/// [JP] 形を足す記録。World が所有する。
		RenderInstance& renderInstance_;
	};
}
