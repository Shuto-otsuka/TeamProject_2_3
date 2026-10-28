#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <PhysicsEngine/Rigidbody/Rigidbody.h>

namespace JPH
{
	class PhysicsSystem;
}

namespace SeedCore
{
	/**
	* [EN]
	* Defines the object layers used for collision filtering. An object
	* layer here is a packed value: the low MOTION_TYPE_BITS bits hold the
	* body's motion-type classification (STATIC/KINEMATIC/DYNAMIC), the bits
	* above them hold the owning Actor's LayerRegistry slot index (see
	* Pack/UnpackMotionType/UnpackUserLayer), the SENSOR bit marks a trigger
	* body, and the PLANAR bit marks a 2D canvas body, so all four axes are
	* encoded into the single value Jolt's broad and narrow phase filter on.
	*
	* 2D and 3D bodies share one Jolt world but never interact: a body only
	* collides with bodies on the same side of the PLANAR bit, regardless of
	* motion type or LayerCollisionMatrix.
	*
	* Motion-type collision matrix (applied in addition to
	* LayerCollisionMatrix's per-Actor-Layer matrix). "t" pairs only when at
	* least one of the two is a trigger (SENSOR):
	*               STATIC  KINEMATIC  DYNAMIC
	*   STATIC         -        t        o
	*   KINEMATIC      t        t        o
	*   DYNAMIC        o        o        o
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 衝突フィルタリングに使うオブジェクトレイヤーの定義。ここでの
	* オブジェクトレイヤーはパックされた値で、下位 MOTION_TYPE_BITS ビットが
	* ボディの運動タイプ分類（STATIC/KINEMATIC/DYNAMIC）を、その上のビットが
	* 所有 Actor の LayerRegistry スロットインデックスを保持し
	* （Pack/UnpackMotionType/UnpackUserLayer 参照）、SENSOR ビットがトリガーの
	* ボディであることを、PLANAR ビットが Canvas の 2D ボディであることを示す。
	* こうして4つの軸すべてを、Jolt の Broad/Narrow Phase がフィルタリングに
	* 使う単一の値へエンコードしている。
	*
	* 2D と 3D のボディは1つの Jolt ワールドを共有するが、互いに干渉しない。
	* ボディは PLANAR ビットが同じ側のボディとしか衝突しない。これは運動タイプや
	* LayerCollisionMatrix に関係なく常に適用される。
	*
	* 運動タイプの衝突マトリクス（LayerCollisionMatrix の Actor レイヤーごとの
	* マトリクスに加えて適用される）。"t" は2つのうち少なくとも一方がトリガー
	* （SENSOR）のときだけ組になる:
	*               STATIC  KINEMATIC  DYNAMIC
	*   STATIC         -        t        o
	*   KINEMATIC      t        t        o
	*   DYNAMIC        o        o        o
	*/
	namespace Layers
	{
		/// [EN] Non-moving geometry (terrain, walls, floors).
		/// [JP] 動かないジオメトリ（地形・壁・床など）。
		SC_CONSTEX ScObjectLayer STATIC = 0;

		/// [EN] Script- or animation-driven bodies (moving platforms, doors); moves but does not respond to physics forces.
		/// [JP] スクリプト/アニメーション制御のボディ（動く床・扉など）。移動するが物理力には反応しない。
		SC_CONSTEX ScObjectLayer KINEMATIC = 1;

		/// [EN] Fully physics-simulated bodies (rigidbodies, ragdolls).
		/// [JP] 物理演算で完全にシミュレートされるボディ（剛体・ラグドールなど）。
		SC_CONSTEX ScObjectLayer DYNAMIC = 2;

		/// [EN] Number of low bits reserved for the motion type.
		/// [JP] 運動タイプ用に確保する下位ビット数。
		SC_CONSTEX Uint32 MOTION_TYPE_BITS = 2;

		/// [EN] Marks a trigger body.
		/// [JP] トリガーボディを示すビット。
		SC_CONSTEX ScObjectLayer SENSOR = static_cast<ScObjectLayer>(1u << 14);

		/// [EN] Marks a body simulated on the 2D canvas.
		/// [JP] 2D Canvas 上でシミュレートするボディを示すビット。
		SC_CONSTEX ScObjectLayer PLANAR = static_cast<ScObjectLayer>(1u << 15);

		/**
		* [EN]
		* Packs motionType (STATIC/KINEMATIC/DYNAMIC) and userLayer (a
		* LayerRegistry slot index) into a single object layer value.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* motionType（STATIC/KINEMATIC/DYNAMIC）と userLayer（LayerRegistry
		* のスロットインデックス）を、単一のオブジェクトレイヤー値へパックする。
		*/
		ScObjectLayer Pack(ScObjectLayer motionType, Size userLayer);

		/**
		* [EN]
		* Extracts the motion type (STATIC/KINEMATIC/DYNAMIC) packed into
		* objectLayer by Pack.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Pack によって objectLayer へパックされた運動タイプ
		* （STATIC/KINEMATIC/DYNAMIC）を取り出す。
		*/
		ScObjectLayer UnpackMotionType(ScObjectLayer objectLayer);

		/**
		* [EN]
		* Extracts the LayerRegistry slot index packed into objectLayer by
		* Pack, ignoring the SENSOR and PLANAR bits.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Pack によって objectLayer へパックされた LayerRegistry
		* スロットインデックスを取り出す。SENSOR と PLANAR のビットは無視する。
		*/
		Size UnpackUserLayer(ScObjectLayer objectLayer);
	}

	/**
	* [EN]
	* Defines the broad-phase layers used for coarse collision culling.
	* STATIC and KINEMATIC share one BP layer because neither moves
	* continuously, so Jolt's broad-phase tree does not need to update them
	* separately. DYNAMIC gets its own BP layer so it can be rebuilt every
	* frame efficiently. SENSOR holds every trigger regardless of motion
	* type, so kinematic bodies can search triggers without searching all
	* static geometry.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 粗い衝突カリングに使うブロードフェーズレイヤーの定義。
	* STATIC と KINEMATIC は同じ BP レイヤーに収める。どちらも連続的には
	* 動かないため、ブロードフェーズツリーを別々に更新する必要がない。
	* DYNAMIC は専用の BP レイヤーを持ち、毎フレーム効率よく再構築される。
	* SENSOR は運動タイプに関係なく全てのトリガーを収め、キネマティックの
	* ボディがスタティックのジオメトリ全体を検索せずにトリガーだけを検索
	* できるようにする。
	*/
	namespace BPLayers
	{
		/// [EN] Static and kinematic bodies.
		/// [JP] スタティックとキネマティックのボディ。
		SC_CONSTEX ScBroadPhaseLayer STATIC = 0;

		/// [EN] Dynamic bodies.
		/// [JP] 動的ボディ。
		SC_CONSTEX ScBroadPhaseLayer DYNAMIC = 1;

		/// [EN] Triggers of every motion type.
		/// [JP] 全ての運動タイプのトリガー。
		SC_CONSTEX ScBroadPhaseLayer SENSOR = 2;

		/// [EN] Number of broad-phase layers exposed to Jolt.
		/// [JP] Jolt へ公開するブロードフェーズレイヤー数。
		SC_CONSTEX Uint32 COUNT = 3;
	}

	/**
	* [EN]
	* Owns Jolt's broad-phase and object-layer filter implementations and
	* uses them to initialize a physics system.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt のブロードフェーズ・オブジェクトレイヤーフィルター実装を所有し、
	* それらを使って物理システムを初期化する。
	*/
	class JoltLayerFilters final
	{
	public:
		/**
		* [EN]
		* Creates the three filters Jolt's physics system needs.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Jolt の物理システムが必要とする3つのフィルターを作る。
		*/
		JoltLayerFilters();

		/**
		* [EN]
		* Defined in the .cpp, where Impl is a complete type.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Impl が完全型になる .cpp 側で定義する。
		*/
		~JoltLayerFilters();

		/// [EN] The physics system keeps references to the filters, so they are neither copied nor reassigned.
		/// [JP] 物理システムがフィルターへの参照を持ち続けるので、コピーも代入もしない。
		JoltLayerFilters(const JoltLayerFilters&) = delete;
		JoltLayerFilters& operator=(const JoltLayerFilters&) = delete;

	public:
		/**
		* [EN]
		* Initializes physicsSystem with these filters and its body and
		* contact capacities. The filters must outlive physicsSystem.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* これらのフィルターとボディ・接触の容量で physicsSystem を初期化する。
		* フィルターは physicsSystem より長く生きている必要がある。
		*/
		void Initialize(JPH::PhysicsSystem& physicsSystem);

	private:
		class Impl;

		/// [EN] The filters themselves, owned through a pointer so Jolt's headers stay out of this one.
		/// [JP] フィルター本体。Jolt のヘッダをこのヘッダから外すため、ポインタ越しに所有する。
		ResourcePtr<Impl> impl_;
	};

	/**
	* [EN]
	* Converts a rigid-body type to the corresponding Jolt motion type.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Rigidbody の種類を対応する Jolt 運動タイプへ変換する。
	*/
	ScMotionType ToMotionType(Rigidbody::BodyType bodyType);

	/**
	* [EN]
	* Overload of ToObjectLayer that packs userLayer (a LayerRegistry
	* slot index) alongside bodyType's motion type.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* userLayer（LayerRegistry のスロットインデックス）を、bodyType の
	* 運動タイプと合わせてパックする ToObjectLayer のオーバーロード。
	*/
	ScObjectLayer ToObjectLayer(Rigidbody::BodyType bodyType, Size userLayer);

	/**
	* [EN]
	* Overload of ToObjectLayer using LayerRegistry::DefaultLayer, for
	* call sites with no Actor/Layer context available.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Actor/Layer の情報が無い呼び出し元向けに、LayerRegistry::DefaultLayer
	* を使う ToObjectLayer のオーバーロード。
	*/
	ScObjectLayer ToObjectLayer(Rigidbody::BodyType bodyType);
}
