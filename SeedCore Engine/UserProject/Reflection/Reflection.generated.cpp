#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <UserProject/Script/BulletController.h>
#include <UserProject/Script/CameraController.h>
#include <UserProject/Script/FloorMoveSystem.h>
#include <UserProject/Script/PendulumController.h>
#include <UserProject/Script/PlayerController.h>

extern "C" int _force_reflection_BulletController = 0;
extern "C" int _force_reflection_CameraController = 0;
extern "C" int _force_reflection_FloorMoveSystem = 0;
extern "C" int _force_reflection_PendulumController = 0;
extern "C" int _force_reflection_PlayerController = 0;

namespace SeedCore
{
	 namespace ScReflection
	 {
		// ---- UserProject/Script/BulletController.h ----
		struct Register_BulletController
		{
			Register_BulletController()
			{
				ReflectionRegistry::Register(String("BulletController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					BulletController& obj = *static_cast<BulletController*>(ptr);
					outInfo.push_back({ String("minSize"), offsetof(BulletController, minSize), AttributeType::Float });
					outInfo.push_back({ String("maxSize"), offsetof(BulletController, maxSize), AttributeType::Float });
					outInfo.push_back({ String("minSpeed"), offsetof(BulletController, minSpeed), AttributeType::Float });
					outInfo.push_back({ String("maxSpeed"), offsetof(BulletController, maxSpeed), AttributeType::Float });
					outInfo.push_back({ String("minAliveTime"), offsetof(BulletController, minAliveTime), AttributeType::Float });
					outInfo.push_back({ String("maxAliveTime"), offsetof(BulletController, maxAliveTime), AttributeType::Float });
					outInfo.push_back({ String("removeDistance"), offsetof(BulletController, removeDistance), AttributeType::Float });
					outInfo.push_back({ String("turnSpeed"), offsetof(BulletController, turnSpeed), AttributeType::Float });
					outInfo.push_back({ String("turnAxis"), offsetof(BulletController, turnAxis), AttributeType::Vector3 });
				});
			}
		};
		static Register_BulletController global_BulletController_register;

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
					outInfo.push_back({ String("horizontalSensitivity"), offsetof(CameraController, horizontalSensitivity), AttributeType::Float });
					outInfo.push_back({ String("verticalSensitivity"), offsetof(CameraController, verticalSensitivity), AttributeType::Float });
					outInfo.push_back({ String("pitchMin"), offsetof(CameraController, pitchMin), AttributeType::Float });
					outInfo.push_back({ String("pitchMax"), offsetof(CameraController, pitchMax), AttributeType::Float });
				});
			}
		};
		static Register_CameraController global_CameraController_register;

		// ---- UserProject/Script/FloorMoveSystem.h ----
		struct Register_FloorMoveSystem
		{
			Register_FloorMoveSystem()
			{
				ReflectionRegistry::Register(String("FloorMoveSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					FloorMoveSystem& obj = *static_cast<FloorMoveSystem*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("動き方");
						fi.offset_ = offsetof(FloorMoveSystem, moveType_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("MoveType");
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("端での待ち時間"), offsetof(FloorMoveSystem, waitTime_), AttributeType::Float });
				});
			}
		};
		static Register_FloorMoveSystem global_FloorMoveSystem_register;

		// ---- UserProject/Script/PendulumController.h ----
		struct Register_PendulumController
		{
			Register_PendulumController()
			{
				ReflectionRegistry::Register(String("PendulumController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					PendulumController& obj = *static_cast<PendulumController*>(ptr);
					outInfo.push_back({ String("rotateSpeed"), offsetof(PendulumController, rotateSpeed), AttributeType::Float });
				});
			}
		};
		static Register_PendulumController global_PendulumController_register;

		// ---- UserProject/Script/PlayerController.h ----
		struct Register_PlayerController
		{
			Register_PlayerController()
			{
				ReflectionRegistry::Register(String("PlayerController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					PlayerController& obj = *static_cast<PlayerController*>(ptr);
					outInfo.push_back({ String("acceleration"), offsetof(PlayerController, acceleration), AttributeType::Float });
					outInfo.push_back({ String("airAcceleration"), offsetof(PlayerController, airAcceleration), AttributeType::Float });
					outInfo.push_back({ String("turnSpeed"), offsetof(PlayerController, turnSpeed), AttributeType::Float });
					outInfo.push_back({ String("jumpEnableTime"), offsetof(PlayerController, jumpEnableTime), AttributeType::Float });
					outInfo.push_back({ String("jumpInputBufferTime"), offsetof(PlayerController, jumpInputBufferTime), AttributeType::Float });
					outInfo.push_back({ String("maxJumpInputTime"), offsetof(PlayerController, maxJumpInputTime), AttributeType::Float });
					outInfo.push_back({ String("jumpGravityScaler"), offsetof(PlayerController, jumpGravityScaler), AttributeType::Float });
					outInfo.push_back({ String("coyoteTime"), offsetof(PlayerController, coyoteTime), AttributeType::Float });
					outInfo.push_back({ String("maxShotChargeTime"), offsetof(PlayerController, maxShotChargeTime), AttributeType::Float });
					outInfo.push_back({ String("bulletOffsetY"), offsetof(PlayerController, bulletOffsetY), AttributeType::Float });
					outInfo.push_back({ String("minBulletOffsetZ"), offsetof(PlayerController, minBulletOffsetZ), AttributeType::Float });
					outInfo.push_back({ String("maxBulletOffsetZ"), offsetof(PlayerController, maxBulletOffsetZ), AttributeType::Float });
					outInfo.push_back({ String("deadPosY"), offsetof(PlayerController, deadPosY), AttributeType::Float });
					outInfo.push_back({ String("maxCostGauge"), offsetof(PlayerController, maxCostGauge), AttributeType::Int });
					outInfo.push_back({ String("minAddCostGauge"), offsetof(PlayerController, minAddCostGauge), AttributeType::Int });
					outInfo.push_back({ String("maxAddCostGauge"), offsetof(PlayerController, maxAddCostGauge), AttributeType::Int });
				});
			}
		};
		static Register_PlayerController global_PlayerController_register;

		struct RegisterEnum_MoveType
		{
			RegisterEnum_MoveType()
			{
				EnumRegistry::Register(String("MoveType"), {
					{ static_cast<Int>(FloorMoveSystem::MoveType::PingPong), String("PingPong") },
					{ static_cast<Int>(FloorMoveSystem::MoveType::Repeat), String("Repeat") },
				});
			}
		};
		static RegisterEnum_MoveType global_MoveType_enum_register;

	}
}