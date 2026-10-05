#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Interop/ShapeInstance.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	class World;

	/**
	* [EN]
	* Gathers the shapes that are not colliders into ShapeDesc entries, once
	* per frame before rendering reads them. Today these are the wireframe
	* debug display (camera frustums, light ranges, audio ranges) and, in
	* debug builds, the physics queries recorded in the World's
	* QueryInstance; shape components drawn as filled surfaces will be
	* gathered here as well.
	* Reads other modules' components as data only, and knows nothing about
	* the GPU, which is ShapeRenderer's side.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* コライダー以外の形を ShapeDesc に集める。描画が読む前に、フレームに
	* 1回行う。今はワイヤーフレームのデバッグ表示（カメラの視錐台、ライトの
	* 範囲、音の範囲）と、Debug ビルドでは World の QueryInstance に記録された
	* 物理クエリで、面で描く形のコンポーネントもここで集める。他の
	* モジュールのコンポーネントはデータとして読むだけで、GPU のことは
	* 知らない（そちらは ShapeRenderer の担当）。
	*/
	class ShapeSystem
	{
	public:
		/**
		* [EN]
		* Rebuilds the shape list from the World. The debug display covers
		* only the selected actors, or every actor while visible is on.
		* aspectRatio is the game view's width over height, for camera
		* frustums.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* World から形の一覧を作り直す。デバッグ表示は選択中のアクターだけ、
		* visible がオンなら全アクターが対象。aspectRatio はゲームビューの
		* 幅÷高さで、カメラの視錐台に使う。
		*/
		void Update(World& world, std::span<const Entity> selectedEntities, Bool visible, Float aspectRatio);

	public:
		/**
		* [EN]
		* Returns the shapes gathered by the last Update.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直前の Update で集めた形を返す。
		*/
		[[nodiscard]] std::span<const ShapeDesc> Shapes()const;

	private:
		/// [EN] How far a camera frustum is drawn at most, so a far plane of hundreds of meters does not fill the view.
		/// [JP] カメラの視錐台を描く奥行きの上限。数百 m 先の far 面で画面が埋まらないようにする。
		SC_CONST Float frustumLength_ = 10.0f;

		/// [EN] Radius of the ring a directional light's rays start from.
		/// [JP] 平行光源の光線が出る円の半径。
		SC_CONST Float directionalRadius_ = 0.5f;

		/// [EN] Length of each parallel ray of a directional light.
		/// [JP] 平行光源の平行な光線1本の長さ。
		SC_CONST Float directionalRayLength_ = 2.0f;

		/// [EN] Number of parallel rays spaced evenly around the ring.
		/// [JP] 円周上に等間隔に並べる平行な光線の本数。
		SC_CONST Uint directionalRayCount_ = 8;

		/// [EN] Upper bound of an arrowhead's length in the world, in meters, so a long arrow (such as a ray that reaches far) keeps a small head.
		/// [JP] ワールドでの矢じりの長さの上限（メートル）。遠くまで届くレイのような長い矢印でも、矢じりが大きくなりすぎないようにする。
		SC_CONST Float worldArrowHeadLength_ = 0.3f;

		/// [EN] Upper bound of an arrowhead's length on the canvas, in pixels.
		/// [JP] Canvas での矢じりの長さの上限（ピクセル）。
		SC_CONST Float canvasArrowHeadLength_ = 15.0f;

		/// [EN] Radius of the marker at a 3D query's hit point, in meters.
		/// [JP] 3D のクエリが当たった点の目印の半径（メートル）。
		SC_CONST Float queryHitRadius_ = 0.05f;

		/// [EN] Length of the surface normal drawn at a 3D query's hit point, in meters.
		/// [JP] 3D のクエリが当たった点に描く面の法線の長さ（メートル）。
		SC_CONST Float queryNormalLength_ = 0.3f;

		/// [EN] Radius of the marker at a 2D query's hit point, in canvas pixels.
		/// [JP] 2D のクエリが当たった点の目印の半径（Canvas のピクセル）。
		SC_CONST Float queryHitPixels_ = 5.0f;

		/// [EN] Length of the surface normal drawn at a 2D query's hit point, in canvas pixels.
		/// [JP] 2D のクエリが当たった点に描く面の法線の長さ（Canvas のピクセル）。
		SC_CONST Float queryNormalPixels_ = 30.0f;

		/// [EN] Shapes gathered this frame; kept between frames so the storage is reused.
		/// [JP] このフレームに集めた形。領域を使い回すため、フレームをまたいで持つ。
		DynamicArray<ShapeDesc> shapes_;
	};
}
