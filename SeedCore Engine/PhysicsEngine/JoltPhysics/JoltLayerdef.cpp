#include <PhysicsEngine/JoltPhysics/JoltLayerdef.h>

namespace SeedCore::Layers
{
	/**
	* [EN]
	* Packs a motion type and Actor layer index into one Jolt object layer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 運動タイプと Actor レイヤーインデックスを1つの Jolt オブジェクトレイヤーへパックする。
	*/
	JPH::ObjectLayer Pack(JPH::ObjectLayer motionType, Size userLayer)
	{
		return static_cast<JPH::ObjectLayer>((static_cast<JPH::uint>(userLayer) << MOTION_TYPE_BITS) | motionType);
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
	JPH::ObjectLayer UnpackMotionType(JPH::ObjectLayer objectLayer)
	{
		return static_cast<JPH::ObjectLayer>(objectLayer & ((1u << MOTION_TYPE_BITS) - 1u));
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
	Size UnpackUserLayer(JPH::ObjectLayer objectLayer)
	{
		return static_cast<Size>((objectLayer & ~(PLANAR | SENSOR)) >> MOTION_TYPE_BITS);
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Returns the number of broad-phase layers exposed to Jolt.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt へ公開するブロードフェーズレイヤー数を返す。
	*/
	JPH::uint BPLayerInterfaceImplementation::GetNumBroadPhaseLayers()const
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
	JPH::BroadPhaseLayer BPLayerInterfaceImplementation::GetBroadPhaseLayer(JPH::ObjectLayer inLayer)const
	{
		/// [EN] Triggers of every motion type share their own BP layer, so only the bodies that need them search it.
		/// [JP] トリガーは運動タイプに関係なく専用の BP レイヤーにまとめ、必要なボディだけがそこを検索するようにする。
		if (inLayer & Layers::SENSOR)
		{
			return BPLayers::SENSOR;
		}

		switch (Layers::UnpackMotionType(inLayer))
		{
		case Layers::STATIC:
			[[fallthrough]];
		case Layers::KINEMATIC:
			return BPLayers::STATIC;
		case Layers::DYNAMIC:
			return BPLayers::DYNAMIC;
		default:
			JPH_ASSERT(false);
			return BPLayers::STATIC;
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
	const Char* BPLayerInterfaceImplementation::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer)const
	{
		switch (static_cast<JPH::uint8>(inLayer))
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
}

namespace SeedCore
{
	/**
	* [EN]
	* Determines whether an object layer can collide with a broad-phase layer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* オブジェクトレイヤーがブロードフェーズレイヤーと衝突可能かを返す。
	*/
	Bool ObjVsBPFilterImplementation::ShouldCollide(JPH::ObjectLayer inLayer, JPH::BroadPhaseLayer inBPLayer)const
	{
		switch (Layers::UnpackMotionType(inLayer))
		{
		case Layers::STATIC:
			[[fallthrough]];
		case Layers::KINEMATIC:
			/// [EN] A kinematic trigger also searches static bodies, since it reports overlaps with everything.
			/// [JP] キネマティックのトリガーは何とでも重なりを知らせるので、スタティックのボディも検索する。
			if (inLayer & Layers::SENSOR)
			{
				return true;
			}

			/// [EN] Any other kinematic body searches triggers as well as dynamic bodies.
			/// [JP] それ以外のキネマティックのボディは、動的ボディに加えてトリガーも検索する。
			return inBPLayer == BPLayers::DYNAMIC || inBPLayer == BPLayers::SENSOR;
		case Layers::DYNAMIC:
			return true;
		default:
			return false;
		}
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Determines whether two packed object layers can collide.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* パックされた2つのオブジェクトレイヤーが衝突可能かを返す。
	*/
	Bool ObjLayerPairFilterImplementation::ShouldCollide(JPH::ObjectLayer inLayerA, JPH::ObjectLayer inLayerB)const
	{
		if ((inLayerA & Layers::PLANAR) != (inLayerB & Layers::PLANAR))
		{
			return false;
		}

		JPH::ObjectLayer motionTypeA = Layers::UnpackMotionType(inLayerA);
		JPH::ObjectLayer motionTypeB = Layers::UnpackMotionType(inLayerB);

		/// [EN] A trigger on either side lets a kinematic body pair with static and kinematic bodies too.
		/// [JP] どちらかがトリガーなら、キネマティックのボディはスタティック・キネマティックのボディとも組になる。
		Bool eitherSensor = ((inLayerA | inLayerB) & Layers::SENSOR) != 0;

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

		return LayerCollisionMatrix::GetCollide(Layers::UnpackUserLayer(inLayerA), Layers::UnpackUserLayer(inLayerB));
	}
}

namespace SeedCore
{
	/**
	* [EN]
	* Converts a rigid-body type to the corresponding Jolt motion type.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Rigidbody の種類を対応する Jolt 運動タイプへ変換する。
	*/
	JPH::EMotionType ToMotionType(Rigidbody::BodyType bodyType)
	{
		switch (bodyType)
		{
		case Rigidbody::BodyType::Dynamic:
			return JPH::EMotionType::Dynamic;
		case Rigidbody::BodyType::Kinematic:
			return JPH::EMotionType::Kinematic;
		case Rigidbody::BodyType::Static:
			return JPH::EMotionType::Static;
		default:
			return JPH::EMotionType::Dynamic;
		}
	}

	/**
	* [EN]
	* Packs a rigid-body motion type and Actor layer into a Jolt object layer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Rigidbody の運動タイプと Actor レイヤーを Jolt オブジェクトレイヤーへパックする。
	*/
	JPH::ObjectLayer ToObjectLayer(Rigidbody::BodyType bodyType, Size userLayer)
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
	JPH::ObjectLayer ToObjectLayer(Rigidbody::BodyType bodyType)
	{
		return ToObjectLayer(bodyType, LayerRegistry::DefaultLayer);
	}
}
