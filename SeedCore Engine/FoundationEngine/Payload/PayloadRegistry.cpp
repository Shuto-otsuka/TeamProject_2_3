#include <FoundationEngine/Payload/PayloadRegistry.h>

// [PAYLOAD_AUTO_BEGIN]
#pragma comment(linker, "/include:_force_payload_AudioSource")
#pragma comment(linker, "/include:_force_payload_Spawner")
#pragma comment(linker, "/include:_force_payload_CameraBrain")
#pragma comment(linker, "/include:_force_payload_AttachmentConstraint")
#pragma comment(linker, "/include:_force_payload_Effector")
#pragma comment(linker, "/include:_force_payload_LookAtConstraint")
#pragma comment(linker, "/include:_force_payload_ParentConstraint")
#pragma comment(linker, "/include:_force_payload_PositionConstraint")
#pragma comment(linker, "/include:_force_payload_RotationConstraint")
#pragma comment(linker, "/include:_force_payload_Text")
#pragma comment(linker, "/include:_force_payload_SkyLight")
#pragma comment(linker, "/include:_force_payload_Mesh")
#pragma comment(linker, "/include:_force_payload_Animator")
#pragma comment(linker, "/include:_force_payload_Material")
#pragma comment(linker, "/include:_force_payload_Skeleton")
#pragma comment(linker, "/include:_force_payload_Movie")
#pragma comment(linker, "/include:_force_payload_Image")
#pragma comment(linker, "/include:_force_payload_MeshCollider")
#pragma comment(linker, "/include:_force_payload_FixedJoint")
#pragma comment(linker, "/include:_force_payload_HingeJoint")
#pragma comment(linker, "/include:_force_payload_SliderJoint")
#pragma comment(linker, "/include:_force_payload_SpringJoint")
// [PAYLOAD_AUTO_END]

namespace SeedCore
{
	FlatMap<String, PayloadRegistry::PayloadFunc>& PayloadRegistry::GetRegistry()
	{
		static FlatMap<String, PayloadFunc> instance;
		return instance;
	}

	void PayloadRegistry::Register(String name, PayloadFunc function)
	{
		GetRegistry()[name] = function;
	}
}
