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
	* [JP]
	* 動き方とレイヤーを、1つの層の番号にまとめる。
	*
	* @brief  層の番号にまとめる
	* @param  motionType 動き方
	* @param  userLayer  エディタで決めたレイヤー
	* @return まとめた層の番号
	*
	* [EN]
	* Packs the way it moves and its layer into one layer number.
	*
	* @brief  Packs a layer number
	* @param  motionType How it moves
	* @param  userLayer  The editor layer
	* @return The packed layer number
	*/
	ScObjectLayer Pack(ScObjectLayer motionType, Size userLayer)
	{
		return static_cast<ScObjectLayer>((static_cast<Uint32>(userLayer) << MOTION_TYPE_BITS) | motionType);
	}

	/**
	* [JP]
	* 層の番号から、動き方だけを取り出す。
	*
	* @brief  動き方を取り出す
	* @param  objectLayer 層の番号
	* @return 動き方
	*
	* [EN]
	* Takes just the way it moves out of a layer number.
	*
	* @brief  Gets the way it moves
	* @param  objectLayer The layer number
	* @return How it moves
	*/
	ScObjectLayer UnpackMotionType(ScObjectLayer objectLayer)
	{
		return static_cast<ScObjectLayer>(objectLayer & ((1u << MOTION_TYPE_BITS) - 1u));
	}

	/**
	* [JP]
	* 層の番号から、エディタで決めたレイヤーだけを取り出す。
	*
	* @brief  レイヤーを取り出す
	* @param  objectLayer 層の番号
	* @return エディタで決めたレイヤー
	*
	* [EN]
	* Takes just the editor layer out of a layer number.
	*
	* @brief  Gets the editor layer
	* @param  objectLayer The layer number
	* @return The editor layer
	*/
	Size UnpackUserLayer(ScObjectLayer objectLayer)
	{
		return static_cast<Size>((objectLayer & ~(PLANAR | SENSOR)) >> MOTION_TYPE_BITS);
	}
}

namespace SeedCore
{
	/**
	* [JP]
	* JoltLayerFilters の中身。ぶつかる組み合わせを決める3つの係を持つ。
	*
	* @brief JoltLayerFilters の中身
	*
	* [EN]
	* The inside of JoltLayerFilters; it holds the three parts that decide what hits what.
	*
	* @brief The inside of JoltLayerFilters
	*/
	class JoltLayerFilters::Impl final
	{
	private:
		/**
		* [JP]
		* 体の層を、大まかな当たり判定の層に振り分ける係。
		*
		* @brief 大まかな層に振り分ける係
		*
		* [EN]
		* Sorts body layers into the rough-check layers.
		*
		* @brief Sorts layers for the rough check
		*/
		class BPLayerInterface final :public JPH::BroadPhaseLayerInterface
		{
		public:
			/**
			* [JP]
			* 大まかな層の数を返す。
			*
			* @brief  層の数を返す
			* @return 層の数
			*
			* [EN]
			* Returns how many rough-check layers there are.
			*
			* @brief  Returns the layer count
			* @return The layer count
			*/
			JPH::uint GetNumBroadPhaseLayers()const override
			{
				return BPLayers::COUNT;
			}

			/**
			* [JP]
			* 体の層が、どの大まかな層に入るかを返す。
			*
			* @brief  大まかな層を返す
			* @param  layer 体の層
			* @return 入る大まかな層
			*
			* [EN]
			* Returns which rough-check layer a body layer goes into.
			*
			* @brief  Returns the rough layer
			* @param  layer The body layer
			* @return The rough layer it goes into
			*/
			JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer)const override
			{
				/// [JP] 重なり用の体は専用の層、それ以外は動くかどうかで分ける
				/// [EN] Overlap bodies get their own layer; the rest split by whether they move
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
			* [JP]
			* 大まかな層の名前を返す。調べるとき用。
			*
			* @brief  層の名前を返す
			* @param  layer 大まかな層
			* @return 層の名前
			*
			* [EN]
			* Returns the name of a rough-check layer, for debugging.
			*
			* @brief  Returns the layer name
			* @param  layer The rough layer
			* @return The layer name
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
		* [JP]
		* 体と大まかな層がぶつかる可能性があるかを決める係。
		*
		* @brief 体と大まかな層の組み合わせを決める係
		*
		* [EN]
		* Decides whether a body could hit a rough-check layer.
		*
		* @brief Decides body vs rough layer
		*/
		class ObjVsBPFilter final :public JPH::ObjectVsBroadPhaseLayerFilter
		{
		public:
			/**
			* [JP]
			* 体と大まかな層がぶつかる可能性があるかを返す。
			*
			* @brief  ぶつかる可能性を返す
			* @param  layer   体の層
			* @param  bpLayer 大まかな層
			* @return ぶつかる可能性があれば true
			* @note   動かないもの同士はぶつからない
			*
			* [EN]
			* Returns whether a body could hit a rough-check layer.
			*
			* @brief  Returns if they could hit
			* @param  layer   The body layer
			* @param  bpLayer The rough layer
			* @return True if they could hit
			* @note   Two still things never hit
			*/
			Bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer bpLayer)const override
			{
				switch (Layers::UnpackMotionType(layer))
				{
				case Layers::STATIC:
					return bpLayer == JPH::BroadPhaseLayer(BPLayers::DYNAMIC);
				case Layers::KINEMATIC:
					if (layer & Layers::SENSOR)
					{
						return true;
					}

					return bpLayer == JPH::BroadPhaseLayer(BPLayers::DYNAMIC) || bpLayer == JPH::BroadPhaseLayer(BPLayers::SENSOR);
				case Layers::DYNAMIC:
					return true;
				default:
					return false;
				}
			}
		};

		/**
		* [JP]
		* 2つの体がぶつかるかを決める係。
		*
		* @brief 体同士の組み合わせを決める係
		*
		* [EN]
		* Decides whether two bodies hit each other.
		*
		* @brief Decides body vs body
		*/
		class ObjLayerPairFilter final :public JPH::ObjectLayerPairFilter
		{
		public:
			/**
			* [JP]
			* 2つの体がぶつかるかを返す。
			*
			* @brief  ぶつかるかを返す
			* @param  layerA 1つめの体の層
			* @param  layerB 2つめの体の層
			* @return ぶつかるなら true
			* @note   最後はエディタのレイヤーの組み合わせ表で決まる
			*
			* [EN]
			* Returns whether two bodies hit each other.
			*
			* @brief  Returns if they hit
			* @param  layerA The first body's layer
			* @param  layerB The second body's layer
			* @return True if they hit
			* @note   The editor's layer table has the final say
			*/
			Bool ShouldCollide(JPH::ObjectLayer layerA, JPH::ObjectLayer layerB)const override
			{
				/// [JP] 2D と 3D はぶつからない
				/// [EN] 2D and 3D never hit
				if ((layerA & Layers::PLANAR) != (layerB & Layers::PLANAR))
				{
					return false;
				}

				/// [JP] 動き方の組み合わせでぶつかるかを決める
				/// [EN] Decide by how the two of them move
				ScObjectLayer motionTypeA = Layers::UnpackMotionType(layerA);
				ScObjectLayer motionTypeB = Layers::UnpackMotionType(layerB);

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

				/// [JP] 最後はエディタのレイヤーの組み合わせ表で決める
				/// [EN] Finally, check the editor's layer table
				return LayerCollisionMatrix::GetCollide(Layers::UnpackUserLayer(layerA), Layers::UnpackUserLayer(layerB));
			}
		};

	public:
		/**
		* [JP]
		* ぶつかる組み合わせを使って、物理エンジンを始める。
		*
		* @brief 物理エンジンを始める
		* @param physicsSystem 始める物理エンジン
		*
		* [EN]
		* Starts the physics engine using these hit rules.
		*
		* @brief Starts the physics engine
		* @param physicsSystem The physics engine to start
		*/
		void Initialize(JPH::PhysicsSystem& physicsSystem)
		{
			physicsSystem.Init(65536, 0, 65536, 65536, bpLayerInterface_, objVsBPFilter_, objLayerPairFilter_);
		}

	private:
		/// [JP] 大まかな層に振り分ける係
		/// [EN] Sorts layers for the rough check
		BPLayerInterface bpLayerInterface_;

		/// [JP] 体と大まかな層の組み合わせを決める係
		/// [EN] Decides body vs rough layer
		ObjVsBPFilter objVsBPFilter_;

		/// [JP] 体同士の組み合わせを決める係
		/// [EN] Decides body vs body
		ObjLayerPairFilter objLayerPairFilter_;
	};

	/**
	* [JP]
	* 中身を作る。
	*
	* @brief 中身を作る
	*
	* [EN]
	* Makes the inner parts.
	*
	* @brief Makes the inner parts
	*/
	JoltLayerFilters::JoltLayerFilters() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	JoltLayerFilters::~JoltLayerFilters() = default;

	/**
	* [JP]
	* ぶつかる組み合わせを使って、物理エンジンを始める。
	*
	* @brief 物理エンジンを始める
	* @param physicsSystem 始める物理エンジン
	*
	* [EN]
	* Starts the physics engine using these hit rules.
	*
	* @brief Starts the physics engine
	* @param physicsSystem The physics engine to start
	*/
	void JoltLayerFilters::Initialize(JPH::PhysicsSystem& physicsSystem)
	{
		impl_->Initialize(physicsSystem);
	}

	/**
	* [JP]
	* Rigidbody の種類を、物理エンジンの動き方に直す。
	*
	* @brief  動き方に直す
	* @param  bodyType Rigidbody の種類
	* @return 動き方
	*
	* [EN]
	* Turns a Rigidbody type into the physics engine's way of moving.
	*
	* @brief  Turns it into a motion type
	* @param  bodyType The Rigidbody type
	* @return How it moves
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
	* [JP]
	* Rigidbody の種類とレイヤーから、層の番号を作る。
	*
	* @brief  層の番号を作る
	* @param  bodyType  Rigidbody の種類
	* @param  userLayer エディタで決めたレイヤー
	* @return 層の番号
	*
	* [EN]
	* Makes a layer number from a Rigidbody type and a layer.
	*
	* @brief  Makes a layer number
	* @param  bodyType  The Rigidbody type
	* @param  userLayer The editor layer
	* @return The layer number
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
	* [JP]
	* Rigidbody の種類と、いつものレイヤーから層の番号を作る。
	*
	* @brief  層の番号を作る
	* @param  bodyType Rigidbody の種類
	* @return 層の番号
	*
	* [EN]
	* Makes a layer number from a Rigidbody type and the default layer.
	*
	* @brief  Makes a layer number
	* @param  bodyType The Rigidbody type
	* @return The layer number
	*/
	ScObjectLayer ToObjectLayer(Rigidbody::BodyType bodyType)
	{
		return ToObjectLayer(bodyType, LayerRegistry::DefaultLayer);
	}
}
