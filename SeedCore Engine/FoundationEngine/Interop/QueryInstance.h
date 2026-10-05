#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Interop/ColliderInstance.h>

namespace SeedCore
{
	/// [EN] Which physics query a QueryDesc records.
	/// [JP] QueryDesc が記録する物理クエリの種類。
	enum class QueryKind :Uint32
	{
		/// [EN] A 3D ray.
		/// [JP] 3D のレイ。
		Raycast,

		/// [EN] A 3D swept sphere.
		/// [JP] 3D の移動する球。
		Spherecast,

		/// [EN] A 3D shape tested for overlaps where it stands.
		/// [JP] その場で重なりを調べる 3D の形。
		Overlap,

		/// [EN] A ray on the canvas.
		/// [JP] Canvas 上のレイ。
		Raycast2D,

		/// [EN] A swept circle on the canvas.
		/// [JP] Canvas 上の移動する円。
		Circlecast2D,

		/// [EN] A canvas shape tested for overlaps where it stands.
		/// [JP] その場で重なりを調べる Canvas 上の形。
		Overlap2D,
	};

	/**
	* [EN]
	* One physics query as it was asked and what it found: the facts only,
	* with nothing about how it is drawn. 3D queries are in world space and
	* meters; 2D queries are in canvas pixels with Y pointing down and z = 0.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 物理クエリ1回分の、問い合わせた内容と結果。事実だけを持ち、描き方の
	* ことは持たない。3D のクエリはワールド空間のメートル、2D のクエリは
	* Y 下向きの Canvas のピクセルで、z は 0。
	*/
	struct QueryDesc
	{
		/// [EN] Which query this is.
		/// [JP] クエリの種類。
		QueryKind kind_ = QueryKind::Raycast;

		/// [EN] Where the ray or sweep starts, or where the overlap shape's center stands.
		/// [JP] レイや掃引の始点。Overlap では形の中心の位置。
		Vector3 origin_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Normalized direction of the ray or sweep; zero for overlaps.
		/// [JP] レイや掃引の向き（正規化済み）。Overlap では 0。
		Vector3 direction_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] How far the ray or sweep reaches.
		/// [JP] レイや掃引が届く距離。
		Float distance_ = 0.0f;

		/// [EN] Radius of the swept sphere or circle.
		/// [JP] 掃引する球や円の半径。
		Float radius_ = 0.0f;

		/// [EN] Shape tested by an overlap.
		/// [JP] Overlap で調べた形。
		ColliderKind shape_ = ColliderKind::Box;

		/// [EN] Size of the overlap shape, in the same layout as ColliderDesc::dimensions_.
		/// [JP] Overlap の形の大きさ。ColliderDesc::dimensions_ と同じ並び。
		Vector3 dimensions_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Rotation of the overlap shape; a 2D one turns about Z by the canvas angle.
		/// [JP] Overlap の形の回転。2D では Canvas の角度だけ Z 軸まわりに回る。
		Quaternion rotation_ = Quaternion::Identity;

		/// [EN] Whether the query found something.
		/// [JP] クエリが何かに当たったか。
		Bool hit_ = false;

		/// [EN] Where the ray or sweep hit.
		/// [JP] レイや掃引が当たった位置。
		Vector3 hitPoint_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Surface normal at the hit.
		/// [JP] 当たった位置の面の法線。
		Vector3 hitNormal_ = { 0.0f, 0.0f, 0.0f };

		/// [EN] Distance travelled until the hit.
		/// [JP] 当たるまでに進んだ距離。
		Float hitDistance_ = 0.0f;
	};

	/**
	* [EN]
	* The physics queries asked since it was last cleared, owned by World.
	* Physics adds each query as it runs, and whoever draws them reads the
	* list and clears it. Sits in FoundationEngine so the module that asks
	* and the module that draws never depend on each other.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前回空にしてから行われた物理クエリの記録。World が所有する。Physics が
	* クエリのたびに足し、描く側が読んで空にする。問い合わせる側と描く側の
	* モジュールが互いに依存しないよう、FoundationEngine に置く。
	*/
	class SEEDCORE_API QueryInstance
	{
	public:
		/**
		* [EN]
		* Records one query.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* クエリを1つ記録する。
		*/
		void Add(const QueryDesc& query);

		/**
		* [EN]
		* Returns the queries recorded since the last Clear.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前回の Clear から記録したクエリを返す。
		*/
		[[nodiscard]] std::span<const QueryDesc> Queries()const;

		/**
		* [EN]
		* Forgets every recorded query.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 記録したクエリをすべて捨てる。
		*/
		void Clear();

	private:
		/// [EN] Recorded queries, oldest first.
		/// [JP] 記録したクエリ。古い順。
		DynamicArray<QueryDesc> queries_;
	};
}
