#include <PhysicsEngine/JoltPhysics/JoltConfig.h>
#include <External/JoltPhysics/Jolt/Jolt.h>
#include <External/JoltPhysics/Jolt/Physics/PhysicsSystem.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/ObjectLayer.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <PhysicsEngine/JoltPhysics/JoltLayerdef.h>
#include <FoundationEngine/World/Layer/LayerRegistry.h>
#include <FoundationEngine/World/Layer/LayerCollisionMatrix.h>

namespace SeedCore::Layers
{
	/**
	* [EN]
	* Packs a motion type and Actor layer index into one object layer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 運動タイプと Actor レイヤーインデックスを1つのオブジェクトレイヤーへパックする。
	*/
	ScObjectLayer Pack(ScObjectLayer motionType, Size userLayer)
	{
		return static_cast<ScObjectLayer>((static_cast<Uint32>(userLayer) << MOTION_TYPE_BITS) | motionType);
	}

	/**
	* [EN]
	* Extracts the packed motion type from an object layer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* オブジェクトレイヤーからパックされた運動タイプを取り出す。
	*/
	ScObjectLayer UnpackMotionType(ScObjectLayer objectLayer)
	{
		return static_cast<ScObjectLayer>(objectLayer & ((1u << MOTION_TYPE_BITS) - 1u));
	}

	/**
	* [EN]
	* Extracts the Actor layer index while ignoring the trigger and planar
	* markers.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* トリガーと 2D のマーカーを無視して Actor レイヤーインデックスを取り出す。
	*/
	Size UnpackUserLayer(ScObjectLayer objectLayer)
	{
		return static_cast<Size>((objectLayer & ~(PLANAR | SENSOR)) >> MOTION_TYPE_BITS);
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* The filters behind JoltLayerFilters. Jolt keeps references to them
	* for as long as the physics system lives, so they are owned here
	* rather than created on the spot.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* JoltLayerFilters の奥にあるフィルター群。物理システムが生きている間
	* Jolt がそれらへの参照を持ち続けるので、その場で作らずここで所有する。
	*/
	class JoltLayerFilters::Impl final
	{
	private:
		/**
		* [EN]
		* Maps each object layer to its corresponding broad-phase layer.
		* Required by JPH::PhysicsSystem::Init.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 各オブジェクトレイヤーを対応するブロードフェーズレイヤーへマッピングする。
		* JPH::PhysicsSystem::Init に必要。
		*/
		class BPLayerInterface final :public JPH::BroadPhaseLayerInterface
		{
		public:
			/**
			* [EN]
			* Returns the number of broad-phase layers exposed to Jolt.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* Jolt へ公開するブロードフェーズレイヤー数を返す。
			*/
			JPH::uint GetNumBroadPhaseLayers()const override
			{
				return BPLayers::COUNT;
			}

			/**
			* [EN]
			* Maps an object layer to its broad-phase layer: a trigger goes to
			* SENSOR, any other body by its motion type.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* オブジェクトレイヤーをブロードフェーズレイヤーへ対応付ける。トリガーは
			* SENSOR へ、それ以外のボディは運動タイプに従って振り分ける。
			*/
			JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer)const override
			{
				/// [EN] Triggers of every motion type share their own BP layer, so only the bodies that need them search it.
				/// [JP] トリガーは運動タイプに関係なく専用の BP レイヤーにまとめ、必要なボディだけがそこを検索するようにする。
				if (layer & Layers::SENSOR)
				{
					return JPH::BroadPhaseLayer(BPLayers::SENSOR);
				}

				switch (Layers::UnpackMotionType(layer))
				{
				case Layers::STATIC:
					FALLTHROUGH;
				case Layers::KINEMATIC:
					return JPH::BroadPhaseLayer(BPLayers::STATIC);
				case Layers::DYNAMIC:
					return JPH::BroadPhaseLayer(BPLayers::DYNAMIC);
				default:
					JPH_ASSERT(false);
					return JPH::BroadPhaseLayer(BPLayers::STATIC);
				}
			}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
			/**
			* [EN]
			* Returns the profiling name of a broad-phase layer.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* ブロードフェーズレイヤーのプロファイル表示名を返す。
			*/
			const Char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer)const override
			{
				switch (static_cast<JPH::uint8>(layer))
				{
				case static_cast<JPH::uint8>(BPLayers::STATIC):
					return "STATIC";
				case static_cast<JPH::uint8>(BPLayers::DYNAMIC):
					return "DYNAMIC";
				case static_cast<JPH::uint8>(BPLayers::SENSOR):
					return "SENSOR";
				default:
					return "UNKNOWN";
				}
			}
#endif
		};

		/**
		* [EN]
		* Determines whether an object layer can collide with a broad-phase layer.
		*
		* STATIC            : only needs to test against DYNAMIC BP layer.
		* KINEMATIC         : tests against DYNAMIC and SENSOR BP layers.
		* KINEMATIC trigger : tests against all BP layers, since it reports
		*                     overlaps with static bodies too.
		* DYNAMIC           : tests against all BP layers.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* オブジェクトレイヤーがブロードフェーズレイヤーと衝突するかを返す。
		*
		* STATIC                  : DYNAMIC BP レイヤーとのみ判定すればよい。
		* KINEMATIC               : DYNAMIC と SENSOR の BP レイヤーと判定する。
		* KINEMATIC のトリガー    : スタティックのボディとの重なりも知らせるので、
		*                           全ての BP レイヤーと判定する。
		* DYNAMIC                 : 全ての BP レイヤーと判定する。
		*/
		class ObjVsBPFilter final :public JPH::ObjectVsBroadPhaseLayerFilter
		{
		public:
			/**
			* [EN]
			* Determines whether an object layer can collide with a broad-phase layer.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* オブジェクトレイヤーがブロードフェーズレイヤーと衝突可能かを返す。
			*/
			Bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer bpLayer)const override
			{
				switch (Layers::UnpackMotionType(layer))
				{
				case Layers::STATIC:
					return bpLayer == JPH::BroadPhaseLayer(BPLayers::DYNAMIC);
				case Layers::KINEMATIC:
					/// [EN] A kinematic trigger also searches static bodies, since it reports overlaps with everything.
					/// [JP] キネマティックのトリガーは何とでも重なりを知らせるので、スタティックのボディも検索する。
					if (layer & Layers::SENSOR)
					{
						return true;
					}

					/// [EN] Any other kinematic body searches triggers as well as dynamic bodies.
					/// [JP] それ以外のキネマティックのボディは、動的ボディに加えてトリガーも検索する。
					return bpLayer == JPH::BroadPhaseLayer(BPLayers::DYNAMIC) || bpLayer == JPH::BroadPhaseLayer(BPLayers::SENSOR);
				case Layers::DYNAMIC:
					return true;
				default:
					return false;
				}
			}
		};

		/**
		* [EN]
		* Determines whether two object layers can collide with each other.
		* Layers on different sides of the PLANAR bit (2D vs 3D) never
		* collide, checked first. Otherwise both the fixed motion-type rules
		* below AND LayerCollisionMatrix's per-Actor-Layer matrix must allow it.
		*
		* STATIC    vs STATIC    : no  (both immovable)
		* STATIC    vs KINEMATIC : only with a trigger (Kinematic pushes Dynamic, not Static, so only overlaps matter)
		* STATIC    vs DYNAMIC   : yes
		* KINEMATIC vs KINEMATIC : only with a trigger (no physics response between script-driven bodies, so only overlaps matter)
		* KINEMATIC vs DYNAMIC   : yes (Kinematic can push Dynamic)
		* DYNAMIC   vs DYNAMIC   : yes
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 2つのオブジェクトレイヤーが互いに衝突するかを返す。PLANAR ビットが
		* 異なるレイヤー同士（2D と 3D）は決して衝突せず、これを最初に判定する。
		* それ以外は、下記の固定された運動タイプルールと、LayerCollisionMatrix の
		* Actor レイヤーごとのマトリクスの両方が許可している必要がある。
		*
		* STATIC    vs STATIC    : しない（どちらも動かない）
		* STATIC    vs KINEMATIC : トリガーのときだけ（Kinematic は Dynamic を押すが Static は押さないので、意味があるのは重なりだけ）
		* STATIC    vs DYNAMIC   : する
		* KINEMATIC vs KINEMATIC : トリガーのときだけ（スクリプト制御同士に物理応答は無いので、意味があるのは重なりだけ）
		* KINEMATIC vs DYNAMIC   : する（Kinematic は Dynamic を押せる）
		* DYNAMIC   vs DYNAMIC   : する
		*/
		class ObjLayerPairFilter final :public JPH::ObjectLayerPairFilter
		{
		public:
			/**
			* [EN]
			* Determines whether two packed object layers can collide.
			*
			* ---------------------------------------------------------------------
			*
			* [JP]
			* パックされた2つのオブジェクトレイヤーが衝突可能かを返す。
			*/
			Bool ShouldCollide(JPH::ObjectLayer layerA, JPH::ObjectLayer layerB)const override
			{
				/// [EN] A 2D body and a 3D body never meet, whatever else the layers say.
				/// [JP] 2D のボディと 3D のボディは、レイヤーの他の設定に関係なく決して組にならない。
				if ((layerA & Layers::PLANAR) != (layerB & Layers::PLANAR))
				{
					return false;
				}

				ScObjectLayer motionTypeA = Layers::UnpackMotionType(layerA);
				ScObjectLayer motionTypeB = Layers::UnpackMotionType(layerB);

				/// [EN] A trigger on either side lets a kinematic body pair with static and kinematic bodies too.
				/// [JP] どちらかがトリガーなら、キネマティックのボディはスタティック・キネマティックのボディとも組になる。
				Bool eitherSensor = ((layerA | layerB) & Layers::SENSOR) != 0;

				Bool motionTypeCollides;
				switch (motionTypeA)
				{
				case Layers::STATIC:
					motionTypeCollides = motionTypeB == Layers::DYNAMIC || (motionTypeB == Layers::KINEMATIC && eitherSensor);
					break;
				case Layers::KINEMATIC:
					motionTypeCollides = motionTypeB == Layers::DYNAMIC || eitherSensor;
					break;
				case Layers::DYNAMIC:
					motionTypeCollides = true;
					break;
				default:
					motionTypeCollides = false;
					break;
				}

				if (!motionTypeCollides)
				{
					return false;
				}

				/// [EN] The motion types allow the pair, so the Actor layers decide.
				/// [JP] 運動タイプとしては組になれるので、最後は Actor のレイヤーで決める。
				return LayerCollisionMatrix::GetCollide(Layers::UnpackUserLayer(layerA), Layers::UnpackUserLayer(layerB));
			}
		};

	public:
		/**
		* [EN]
		* Initializes physicsSystem with these filters and its body and
		* contact capacities.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* これらのフィルターとボディ・接触の容量で physicsSystem を初期化する。
		*/
		void Initialize(JPH::PhysicsSystem& physicsSystem)
		{
			/// [EN] Up to 65536 bodies, the default body mutex count, 65536 body pairs and 65536 contact constraints.
			/// [JP] ボディ最大 65536、ボディのミューテックス数は既定、ボディペア 65536、接触拘束 65536。
			physicsSystem.Init(65536, 0, 65536, 65536, bpLayerInterface_, objVsBPFilter_, objLayerPairFilter_);
		}

	private:
		/// [EN] Object layer to broad-phase layer mapping.
		/// [JP] オブジェクトレイヤーからブロードフェーズレイヤーへの対応付け。
		BPLayerInterface bpLayerInterface_;

		/// [EN] Object layer versus broad-phase layer filter.
		/// [JP] オブジェクトレイヤーとブロードフェーズレイヤーのフィルター。
		ObjVsBPFilter objVsBPFilter_;

		/// [EN] Object layer versus object layer filter.
		/// [JP] オブジェクトレイヤー同士のフィルター。
		ObjLayerPairFilter objLayerPairFilter_;
	};

	/**
	* [EN]
	* Creates the three filters Jolt's physics system needs.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt の物理システムが必要とする3つのフィルターを作る。
	*/
	JoltLayerFilters::JoltLayerFilters() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	/**
	* [EN]
	* Defined here, where Impl is a complete type.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Impl が完全型になるここで定義する。
	*/
	JoltLayerFilters::~JoltLayerFilters() = default;

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
	void JoltLayerFilters::Initialize(JPH::PhysicsSystem& physicsSystem)
	{
		impl_->Initialize(physicsSystem);
	}

	/**
	* [EN]
	* Converts a rigid-body type to the corresponding Jolt motion type.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Rigidbody の種類を対応する Jolt 運動タイプへ変換する。
	*/
	ScMotionType ToMotionType(Rigidbody::BodyType bodyType)
	{
		switch (bodyType)
		{
		case Rigidbody::BodyType::Dynamic:
			return ScMotionType::Dynamic;
		case Rigidbody::BodyType::Kinematic:
			return ScMotionType::Kinematic;
		case Rigidbody::BodyType::Static:
			return ScMotionType::Static;
		default:
			return ScMotionType::Dynamic;
		}
	}

	/**
	* [EN]
	* Packs a rigid-body motion type and Actor layer into an object layer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Rigidbody の運動タイプと Actor レイヤーをオブジェクトレイヤーへパックする。
	*/
	ScObjectLayer ToObjectLayer(Rigidbody::BodyType bodyType, Size userLayer)
	{
		switch (bodyType)
		{
		case Rigidbody::BodyType::Dynamic:
			return Layers::Pack(Layers::DYNAMIC, userLayer);
		case Rigidbody::BodyType::Kinematic:
			return Layers::Pack(Layers::KINEMATIC, userLayer);
		case Rigidbody::BodyType::Static:
			return Layers::Pack(Layers::STATIC, userLayer);
		default:
			return Layers::Pack(Layers::DYNAMIC, userLayer);
		}
	}

	/**
	* [EN]
	* Converts a rigid-body type using the default Actor layer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 既定の Actor レイヤーを使って Rigidbody の種類を変換する。
	*/
	ScObjectLayer ToObjectLayer(Rigidbody::BodyType bodyType)
	{
		return ToObjectLayer(bodyType, LayerRegistry::DefaultLayer);
	}
}
