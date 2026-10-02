#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <UserProject/Script/CameraController.h>
#include <UserProject/Script/MoveStopController.h>
#include <UserProject/Script/PlayerController.h>

extern "C" int _force_reflection_CameraController = 0;
extern "C" int _force_reflection_MoveStopController = 0;
extern "C" int _force_reflection_PlayerController = 0;

namespace SeedCore
{
	 namespace ScReflection
	 {
		// ---- UserProject/Script/CameraController.h ----
		struct Register_CameraController
		{
			Register_CameraController()
			{
				ReflectionRegistry::Register(String("CameraController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CameraController& obj = *static_cast<CameraController*>(ptr);
					outInfo.push_back({ String("smoothFocusSpeed"), offsetof(CameraController, smoothFocusSpeed), AttributeType::Float });
					outInfo.push_back({ String("lookPlayerHeight"), offsetof(CameraController, lookPlayerHeight), AttributeType::Float });
					outInfo.push_back({ String("distance"), offsetof(CameraController, distance), AttributeType::Float });
					outInfo.push_back({ String("minDistance"), offsetof(CameraController, minDistance), AttributeType::Float });
					outInfo.push_back({ String("maxDistance"), offsetof(CameraController, maxDistance), AttributeType::Float });
					outInfo.push_back({ String("zoomSpeed"), offsetof(CameraController, zoomSpeed), AttributeType::Float });
					outInfo.push_back({ String("sensitivity"), offsetof(CameraController, sensitivity), AttributeType::Float });
					outInfo.push_back({ String("pitchMin"), offsetof(CameraController, pitchMin), AttributeType::Float });
					outInfo.push_back({ String("pitchMax"), offsetof(CameraController, pitchMax), AttributeType::Float });
				});
			}
		};
		static Register_CameraController global_CameraController_register;

		// ---- UserProject/Script/MoveStopController.h ----
		struct Register_MoveStopController
		{
			Register_MoveStopController()
			{
				ReflectionRegistry::Register(String("MoveStopController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					MoveStopController& obj = *static_cast<MoveStopController*>(ptr);
					outInfo.push_back({ String("moveDirection"), offsetof(MoveStopController, moveDirection), AttributeType::Unknown });
				});
			}
		};
		static Register_MoveStopController global_MoveStopController_register;

		// ---- UserProject/Script/PlayerController.h ----
		struct Register_PlayerController
		{
			Register_PlayerController()
			{
				ReflectionRegistry::Register(String("PlayerController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					PlayerController& obj = *static_cast<PlayerController*>(ptr);
					outInfo.push_back({ String("turnSpeed"), offsetof(PlayerController, turnSpeed), AttributeType::Float });
					outInfo.push_back({ String("minJumpPower"), offsetof(PlayerController, minJumpPower), AttributeType::Float });
					outInfo.push_back({ String("maxJumpPower"), offsetof(PlayerController, maxJumpPower), AttributeType::Float });
					outInfo.push_back({ String("maxJumpInputTime"), offsetof(PlayerController, maxJumpInputTime), AttributeType::Float });
					outInfo.push_back({ String("jumpEnableTime"), offsetof(PlayerController, jumpEnableTime), AttributeType::Float });
					outInfo.push_back({ String("coyoteTime"), offsetof(PlayerController, coyoteTime), AttributeType::Float });
				});
			}
		};
		static Register_PlayerController global_PlayerController_register;

	}
}