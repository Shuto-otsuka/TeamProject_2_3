#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <UserProject/Script/PlayerController.h>

extern "C" int _force_reflection_PlayerController = 0;

namespace SeedCore
{
	 namespace ScReflection
	 {
		// ---- UserProject/Script/PlayerController.h ----
		struct Register_PlayerController
		{
			Register_PlayerController()
			{
				ReflectionRegistry::Register(String("PlayerController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					PlayerController& obj = *static_cast<PlayerController*>(ptr);
					outInfo.push_back({ String("acceleration"), offsetof(PlayerController, acceleration), AttributeType::Float });
				});
			}
		};
		static Register_PlayerController global_PlayerController_register;

	}
}