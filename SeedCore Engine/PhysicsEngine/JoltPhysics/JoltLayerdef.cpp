#include <External/JoltPhysics/Jolt/Jolt.h>
#include <External/JoltPhysics/Jolt/Physics/PhysicsSystem.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/ObjectLayer.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <PhysicsEngine/JoltPhysics/JoltLayerdef.h>
#include <FoundationEngine/World/Layer/LayerRegistry.h>
#include <FoundationEngine/World/Layer/LayerCollisionMatrix.h>

namespace SeedCore::Layers
{
	ScObjectLayer Pack(ScObjectLayer motionType, Size userLayer)
	{
		return static_cast<ScObjectLayer>((static_cast<Uint32>(userLayer) << MOTION_TYPE_BITS) | motionType);
	}

	ScObjectLayer UnpackMotionType(ScObjectLayer objectLayer)
	{
		return static_cast<ScObjectLayer>(objectLayer & ((1u << MOTION_TYPE_BITS) - 1u));
	}

	Size UnpackUserLayer(ScObjectLayer objectLayer)
	{
		return static_cast<Size>((objectLayer & ~(PLANAR | SENSOR)) >> MOTION_TYPE_BITS);
	}
}

namespace SeedCore
{
	class JoltLayerFilters::Impl final
	{
	private:
		class BPLayerInterface final :public JPH::BroadPhaseLayerInterface
		{
		public:
			JPH::uint GetNumBroadPhaseLayers()const override
			{
				return BPLayers::COUNT;
			}

			JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer)const override
			{
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

		class ObjVsBPFilter final :public JPH::ObjectVsBroadPhaseLayerFilter
		{
		public:
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

		class ObjLayerPairFilter final :public JPH::ObjectLayerPairFilter
		{
		public:
			Bool ShouldCollide(JPH::ObjectLayer layerA, JPH::ObjectLayer layerB)const override
			{
				if ((layerA & Layers::PLANAR) != (layerB & Layers::PLANAR))
				{
					return false;
				}

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

				return LayerCollisionMatrix::GetCollide(Layers::UnpackUserLayer(layerA), Layers::UnpackUserLayer(layerB));
			}
		};

	public:
		void Initialize(JPH::PhysicsSystem& physicsSystem)
		{
			physicsSystem.Init(65536, 0, 65536, 65536, bpLayerInterface_, objVsBPFilter_, objLayerPairFilter_);
		}

	private:
		BPLayerInterface bpLayerInterface_;
		ObjVsBPFilter objVsBPFilter_;
		ObjLayerPairFilter objLayerPairFilter_;
	};

	JoltLayerFilters::JoltLayerFilters() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	JoltLayerFilters::~JoltLayerFilters() = default;

	void JoltLayerFilters::Initialize(JPH::PhysicsSystem& physicsSystem)
	{
		impl_->Initialize(physicsSystem);
	}

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

	ScObjectLayer ToObjectLayer(Rigidbody::BodyType bodyType)
	{
		return ToObjectLayer(bodyType, LayerRegistry::DefaultLayer);
	}
}
