#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Reflection/ReflectionRegistry.h>
#include <UserProject/Script/AreaController.h>
#include <UserProject/Script/BallController.h>
#include <UserProject/Script/BallReseterController.h>
#include <UserProject/Script/BombController.h>
#include <UserProject/Script/BombExplosionController.h>
#include <UserProject/Script/BulletController.h>
#include <UserProject/Script/BulletNaviController.h>
#include <UserProject/Script/CameraController.h>
#include <UserProject/Script/ChargeGaugeController.h>
#include <UserProject/Script/CheckPointAreaController.h>
#include <UserProject/Script/CircuitSystem.h>
#include <UserProject/Script/CostGaugeController.h>
#include <UserProject/Script/DoorSystem.h>
#include <UserProject/Script/FirstPersonCameraController.h>
#include <UserProject/Script/FloorMoveSystem.h>
#include <UserProject/Script/GoalSystem.h>
#include <UserProject/Script/HintSystem.h>
#include <UserProject/Script/LaserController.h>
#include <UserProject/Script/LightSwitchSystem.h>
#include <UserProject/Script/PendulumController.h>
#include <UserProject/Script/PlayerController.h>
#include <UserProject/Script/StageSelectButtonController.h>
#include <UserProject/Script/StopController.h>
#include <UserProject/Script/SwitchSystem.h>
#include <UserProject/Script/TitleLogoController.h>

extern "C" int _force_reflection_AreaController = 0;
extern "C" int _force_reflection_BallController = 0;
extern "C" int _force_reflection_BallReseterController = 0;
extern "C" int _force_reflection_BombController = 0;
extern "C" int _force_reflection_BombExplosionController = 0;
extern "C" int _force_reflection_BulletController = 0;
extern "C" int _force_reflection_BulletNaviController = 0;
extern "C" int _force_reflection_CameraController = 0;
extern "C" int _force_reflection_ChargeGaugeController = 0;
extern "C" int _force_reflection_CheckPointAreaController = 0;
extern "C" int _force_reflection_CircuitSystem = 0;
extern "C" int _force_reflection_CostGaugeController = 0;
extern "C" int _force_reflection_DoorSystem = 0;
extern "C" int _force_reflection_FirstPersonCameraController = 0;
extern "C" int _force_reflection_FloorMoveSystem = 0;
extern "C" int _force_reflection_GoalSystem = 0;
extern "C" int _force_reflection_HintSystem = 0;
extern "C" int _force_reflection_LaserController = 0;
extern "C" int _force_reflection_LightSwitchSystem = 0;
extern "C" int _force_reflection_PendulumController = 0;
extern "C" int _force_reflection_PlayerController = 0;
extern "C" int _force_reflection_StageSelectButtonController = 0;
extern "C" int _force_reflection_StopController = 0;
extern "C" int _force_reflection_SwitchSystem = 0;
extern "C" int _force_reflection_TitleLogoController = 0;

namespace SeedCore
{
	 namespace ScReflection
	 {
		// ---- UserProject/Script/AreaController.h ----
		struct Register_AreaController
		{
			Register_AreaController()
			{
				ReflectionRegistry::Register(String("AreaController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AreaController& obj = *static_cast<AreaController*>(ptr);
					outInfo.push_back({ String("fadeSpeed"), offsetof(AreaController, fadeSpeed), AttributeType::Float });
				});
			}
		};
		static Register_AreaController global_AreaController_register;

		// ---- UserProject/Script/BallController.h ----
		struct Register_BallController
		{
			Register_BallController()
			{
				ReflectionRegistry::Register(String("BallController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					BallController& obj = *static_cast<BallController*>(ptr);
					outInfo.push_back({ String("respawnHeight"), offsetof(BallController, respawnHeight), AttributeType::Float });
					outInfo.push_back({ String("stopAirResistance"), offsetof(BallController, stopAirResistance), AttributeType::Float });
				});
			}
		};
		static Register_BallController global_BallController_register;

		// ---- UserProject/Script/BallReseterController.h ----
		struct Register_BallReseterController
		{
			Register_BallReseterController()
			{
				ReflectionRegistry::Register(String("BallReseterController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					BallReseterController& obj = *static_cast<BallReseterController*>(ptr);
					outInfo.push_back({ String("turnSpeed"), offsetof(BallReseterController, turnSpeed), AttributeType::Float });
				});
			}
		};
		static Register_BallReseterController global_BallReseterController_register;

		// ---- UserProject/Script/BombController.h ----
		struct Register_BombController
		{
			Register_BombController()
			{
				ReflectionRegistry::Register(String("BombController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					BombController& obj = *static_cast<BombController*>(ptr);
					outInfo.push_back({ String("moveX"), offsetof(BombController, moveX), AttributeType::Float });
					outInfo.push_back({ String("moveZ"), offsetof(BombController, moveZ), AttributeType::Float });
					outInfo.push_back({ String("moveSpeed"), offsetof(BombController, moveSpeed), AttributeType::Float });
					outInfo.push_back({ String("turnSpeed"), offsetof(BombController, turnSpeed), AttributeType::Float });
					outInfo.push_back({ String("explosionInterval"), offsetof(BombController, explosionInterval), AttributeType::Float });
					outInfo.push_back({ String("explosionRadius"), offsetof(BombController, explosionRadius), AttributeType::Float });
					outInfo.push_back({ String("startX"), offsetof(BombController, startX), AttributeType::Bool });
					outInfo.push_back({ String("timerOffset"), offsetof(BombController, timerOffset), AttributeType::Vector3 });
				});
			}
		};
		static Register_BombController global_BombController_register;

		// ---- UserProject/Script/BombExplosionController.h ----
		struct Register_BombExplosionController
		{
			Register_BombExplosionController()
			{
				ReflectionRegistry::Register(String("BombExplosionController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					BombExplosionController& obj = *static_cast<BombExplosionController*>(ptr);
					outInfo.push_back({ String("scaleUpTime"), offsetof(BombExplosionController, scaleUpTime), AttributeType::Float });
					outInfo.push_back({ String("scaleDownTime"), offsetof(BombExplosionController, scaleDownTime), AttributeType::Float });
					outInfo.push_back({ String("waitTime"), offsetof(BombExplosionController, waitTime), AttributeType::Float });
				});
			}
		};
		static Register_BombExplosionController global_BombExplosionController_register;

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
					outInfo.push_back({ String("turnSpeed"), offsetof(BulletController, turnSpeed), AttributeType::Float });
					outInfo.push_back({ String("turnAxis"), offsetof(BulletController, turnAxis), AttributeType::Vector3 });
					outInfo.push_back({ String("stopTimerOffset"), offsetof(BulletController, stopTimerOffset), AttributeType::Vector3 });
				});
			}
		};
		static Register_BulletController global_BulletController_register;

		// ---- UserProject/Script/BulletNaviController.h ----
		struct Register_BulletNaviController
		{
			Register_BulletNaviController()
			{
				ReflectionRegistry::Register(String("BulletNaviController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					BulletNaviController& obj = *static_cast<BulletNaviController*>(ptr);
					outInfo.push_back({ String("interval"), offsetof(BulletNaviController, interval), AttributeType::Float });
					outInfo.push_back({ String("scrollSpeed"), offsetof(BulletNaviController, scrollSpeed), AttributeType::Float });
				});
			}
		};
		static Register_BulletNaviController global_BulletNaviController_register;

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

		// ---- UserProject/Script/ChargeGaugeController.h ----
		struct Register_ChargeGaugeController
		{
			Register_ChargeGaugeController()
			{
				ReflectionRegistry::Register(String("ChargeGaugeController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					ChargeGaugeController& obj = *static_cast<ChargeGaugeController*>(ptr);
					outInfo.push_back({ String("posXMin"), offsetof(ChargeGaugeController, posXMin), AttributeType::Float });
					outInfo.push_back({ String("posXMax"), offsetof(ChargeGaugeController, posXMax), AttributeType::Float });
					outInfo.push_back({ String("sizeXMin"), offsetof(ChargeGaugeController, sizeXMin), AttributeType::Float });
					outInfo.push_back({ String("sizeXMax"), offsetof(ChargeGaugeController, sizeXMax), AttributeType::Float });
				});
			}
		};
		static Register_ChargeGaugeController global_ChargeGaugeController_register;

		// ---- UserProject/Script/CheckPointAreaController.h ----
		struct Register_CheckPointAreaController
		{
			Register_CheckPointAreaController()
			{
				ReflectionRegistry::Register(String("CheckPointAreaController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CheckPointAreaController& obj = *static_cast<CheckPointAreaController*>(ptr);
					outInfo.push_back({ String("何個目のチェックポイントか"), offsetof(CheckPointAreaController, step), AttributeType::Int });
					outInfo.push_back({ String("firstPerson"), offsetof(CheckPointAreaController, firstPerson), AttributeType::Bool });
				});
			}
		};
		static Register_CheckPointAreaController global_CheckPointAreaController_register;

		// ---- UserProject/Script/CircuitSystem.h ----
		struct Register_CircuitSystem
		{
			Register_CircuitSystem()
			{
				ReflectionRegistry::Register(String("CircuitSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CircuitSystem& obj = *static_cast<CircuitSystem*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("論理");
						fi.offset_ = offsetof(CircuitSystem, logic_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("CircuitSystem::Logic");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("起動最低数");
						fi.offset_ = offsetof(CircuitSystem, guaranteeCount_);
						fi.type_ = AttributeType::Int;
						fi.clampMin_ = 0;
						fi.clampMax_ = 100000;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("結果を反転"), offsetof(CircuitSystem, flipResult_), AttributeType::Bool });
					outInfo.push_back({ String("結果を保持"), offsetof(CircuitSystem, keepResult_), AttributeType::Bool });
				});
			}
		};
		static Register_CircuitSystem global_CircuitSystem_register;

		// ---- UserProject/Script/CostGaugeController.h ----
		struct Register_CostGaugeController
		{
			Register_CostGaugeController()
			{
				ReflectionRegistry::Register(String("CostGaugeController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CostGaugeController& obj = *static_cast<CostGaugeController*>(ptr);
					outInfo.push_back({ String("posXMin"), offsetof(CostGaugeController, posXMin), AttributeType::Float });
					outInfo.push_back({ String("posXMax"), offsetof(CostGaugeController, posXMax), AttributeType::Float });
					outInfo.push_back({ String("sizeXMin"), offsetof(CostGaugeController, sizeXMin), AttributeType::Float });
					outInfo.push_back({ String("sizeXMax"), offsetof(CostGaugeController, sizeXMax), AttributeType::Float });
					outInfo.push_back({ String("gaugeMoveSpeed"), offsetof(CostGaugeController, gaugeMoveSpeed), AttributeType::Float });
				});
			}
		};
		static Register_CostGaugeController global_CostGaugeController_register;

		// ---- UserProject/Script/DoorSystem.h ----
		struct Register_DoorSystem
		{
			Register_DoorSystem()
			{
				ReflectionRegistry::Register(String("DoorSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					DoorSystem& obj = *static_cast<DoorSystem*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("沈む距離");
						fi.offset_ = offsetof(DoorSystem, sinkDistance_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("開くまでの時間");
						fi.offset_ = offsetof(DoorSystem, openTime_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("閉まるまでの時間");
						fi.offset_ = offsetof(DoorSystem, closeTime_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_DoorSystem global_DoorSystem_register;

		// ---- UserProject/Script/FirstPersonCameraController.h ----
		struct Register_FirstPersonCameraController
		{
			Register_FirstPersonCameraController()
			{
				ReflectionRegistry::Register(String("FirstPersonCameraController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					FirstPersonCameraController& obj = *static_cast<FirstPersonCameraController*>(ptr);
					outInfo.push_back({ String("offset"), offsetof(FirstPersonCameraController, offset), AttributeType::Vector3 });
				});
			}
		};
		static Register_FirstPersonCameraController global_FirstPersonCameraController_register;

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
						fi.enum_.typeName_ = String("FloorMoveSystem::MoveType");
						outInfo.push_back(std::move(fi));
					}
					{
						auto& arr = obj.localPositions_;
						FieldInfo header;
						header.name_ = String("床からのProbeの相対位置");
						header.offset_ = 0;
						header.type_ = AttributeType::Vector3;
						header.array_.size_ = arr.size();
						header.array_.add_ = [&obj]() { obj.localPositions_.push_back({}); };
						header.array_.remove_ = [&obj](Size idx) { if (idx < obj.localPositions_.size()) obj.localPositions_.erase(obj.localPositions_.begin() + idx); };
						outInfo.push_back(std::move(header));
						for (Size i = 0; i < arr.size(); ++i)
						{
							outInfo.push_back({ String("[" + std::to_string(i) + "]"), 0, AttributeType::Vector3, PayloadType::None, &arr[i] });
						}
					}
					{
						FieldInfo fi;
						fi.name_ = String("速度");
						fi.offset_ = offsetof(FloorMoveSystem, scalarSpeed_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("端での待ち時間"), offsetof(FloorMoveSystem, waitTime_), AttributeType::Float });
					outInfo.push_back({ String("電力を用いるか"), offsetof(FloorMoveSystem, isCanCircuit_), AttributeType::Bool });
					outInfo.push_back({ String("デバッグアローを描画するか"), offsetof(FloorMoveSystem, isDebugArrowDraw_), AttributeType::Bool });
				});
			}
		};
		static Register_FloorMoveSystem global_FloorMoveSystem_register;

		// ---- UserProject/Script/GoalSystem.h ----
		struct Register_GoalSystem
		{
			Register_GoalSystem()
			{
				ReflectionRegistry::Register(String("GoalSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					GoalSystem& obj = *static_cast<GoalSystem*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("範囲");
						fi.offset_ = offsetof(GoalSystem, range_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_GoalSystem global_GoalSystem_register;

		// ---- UserProject/Script/HintSystem.h ----
		struct Register_HintSystem
		{
			Register_HintSystem()
			{
				ReflectionRegistry::Register(String("HintSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					HintSystem& obj = *static_cast<HintSystem*>(ptr);
					outInfo.push_back({ String("回転速度"), offsetof(HintSystem, rotateSpeed_), AttributeType::Float });
					outInfo.push_back({ String("表示維持時間"), offsetof(HintSystem, maintainTime_), AttributeType::Float });
					outInfo.push_back({ String("フェード時間"), offsetof(HintSystem, fadeTime_), AttributeType::Float });
					outInfo.push_back({ String("ヒント文"), offsetof(HintSystem, hintText_), AttributeType::String });
				});
			}
		};
		static Register_HintSystem global_HintSystem_register;

		// ---- UserProject/Script/LaserController.h ----
		struct Register_LaserController
		{
			Register_LaserController()
			{
				ReflectionRegistry::Register(String("LaserController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					LaserController& obj = *static_cast<LaserController*>(ptr);
					outInfo.push_back({ String("centerOffset"), offsetof(LaserController, centerOffset), AttributeType::Vector3 });
					outInfo.push_back({ String("deltaOffset"), offsetof(LaserController, deltaOffset), AttributeType::Vector3 });
					outInfo.push_back({ String("length"), offsetof(LaserController, length), AttributeType::Float });
					outInfo.push_back({ String("tickness"), offsetof(LaserController, tickness), AttributeType::Float });
				});
			}
		};
		static Register_LaserController global_LaserController_register;

		// ---- UserProject/Script/LightSwitchSystem.h ----
		struct Register_LightSwitchSystem
		{
			Register_LightSwitchSystem()
			{
				ReflectionRegistry::Register(String("LightSwitchSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					LightSwitchSystem& obj = *static_cast<LightSwitchSystem*>(ptr);
					outInfo.push_back({ String("回転速度"), offsetof(LightSwitchSystem, rotateSpeed_), AttributeType::Float });
					outInfo.push_back({ String("反応距離"), offsetof(LightSwitchSystem, detectDistance_), AttributeType::Float });
					outInfo.push_back({ String("点灯を保持"), offsetof(LightSwitchSystem, keepResult_), AttributeType::Bool });
				});
			}
		};
		static Register_LightSwitchSystem global_LightSwitchSystem_register;

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
					outInfo.push_back({ String("bulletRemoveDistance"), offsetof(PlayerController, bulletRemoveDistance), AttributeType::Float });
					outInfo.push_back({ String("deadPosY"), offsetof(PlayerController, deadPosY), AttributeType::Float });
					outInfo.push_back({ String("firstPersonSensitivity"), offsetof(PlayerController, firstPersonSensitivity), AttributeType::Float });
					outInfo.push_back({ String("firstPersonMinPitch"), offsetof(PlayerController, firstPersonMinPitch), AttributeType::Float });
					outInfo.push_back({ String("firstPersonMaxPitch"), offsetof(PlayerController, firstPersonMaxPitch), AttributeType::Float });
					outInfo.push_back({ String("resetInputTime"), offsetof(PlayerController, resetInputTime), AttributeType::Float });
					outInfo.push_back({ String("bulletMinSize"), offsetof(PlayerController, bulletMinSize), AttributeType::Float });
					outInfo.push_back({ String("bulletMaxSize"), offsetof(PlayerController, bulletMaxSize), AttributeType::Float });
					outInfo.push_back({ String("maxCostGauge"), offsetof(PlayerController, maxCostGauge), AttributeType::Int });
					outInfo.push_back({ String("minAddCostGauge"), offsetof(PlayerController, minAddCostGauge), AttributeType::Int });
					outInfo.push_back({ String("maxAddCostGauge"), offsetof(PlayerController, maxAddCostGauge), AttributeType::Int });
				});
			}
		};
		static Register_PlayerController global_PlayerController_register;

		// ---- UserProject/Script/StageSelectButtonController.h ----
		struct Register_StageSelectButtonController
		{
			Register_StageSelectButtonController()
			{
				ReflectionRegistry::Register(String("StageSelectButtonController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					StageSelectButtonController& obj = *static_cast<StageSelectButtonController*>(ptr);
					outInfo.push_back({ String("stageNumber"), offsetof(StageSelectButtonController, stageNumber), AttributeType::Int });
					outInfo.push_back({ String("onCursorColor"), offsetof(StageSelectButtonController, onCursorColor), AttributeType::Color });
					outInfo.push_back({ String("clickedColor"), offsetof(StageSelectButtonController, clickedColor), AttributeType::Color });
					outInfo.push_back({ String("colorChangeSpeed"), offsetof(StageSelectButtonController, colorChangeSpeed), AttributeType::Float });
					outInfo.push_back({ String("clickedSize"), offsetof(StageSelectButtonController, clickedSize), AttributeType::Float });
					outInfo.push_back({ String("onCursorSize"), offsetof(StageSelectButtonController, onCursorSize), AttributeType::Float });
					outInfo.push_back({ String("sizeChangeSpeed"), offsetof(StageSelectButtonController, sizeChangeSpeed), AttributeType::Float });
					outInfo.push_back({ String("sceneChangeWaitTime"), offsetof(StageSelectButtonController, sceneChangeWaitTime), AttributeType::Float });
				});
			}
		};
		static Register_StageSelectButtonController global_StageSelectButtonController_register;

		// ---- UserProject/Script/StopController.h ----
		struct Register_StopController
		{
			Register_StopController()
			{
				ReflectionRegistry::Register(String("StopController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					StopController& obj = *static_cast<StopController*>(ptr);
					outInfo.push_back({ String("stopTimerOffset"), offsetof(StopController, stopTimerOffset), AttributeType::Vector3 });
				});
			}
		};
		static Register_StopController global_StopController_register;

		// ---- UserProject/Script/SwitchSystem.h ----
		struct Register_SwitchSystem
		{
			Register_SwitchSystem()
			{
				ReflectionRegistry::Register(String("SwitchSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					SwitchSystem& obj = *static_cast<SwitchSystem*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("動作");
						fi.offset_ = offsetof(SwitchSystem, mode_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("SwitchSystem::Mode");
						outInfo.push_back(std::move(fi));
					}
					{
						FieldInfo fi;
						fi.name_ = String("判定の形");
						fi.offset_ = offsetof(SwitchSystem, shape_);
						fi.type_ = AttributeType::Enum;
						fi.enum_.typeName_ = String("SwitchSystem::Shape");
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("判定サイズ"), offsetof(SwitchSystem, detectSize_), AttributeType::Vector3 });
					outInfo.push_back({ String("判定オフセット"), offsetof(SwitchSystem, detectOffset_), AttributeType::Vector3 });
					{
						auto& arr = obj.targetTags_;
						FieldInfo header;
						header.name_ = String("対象タグ");
						header.offset_ = 0;
						header.type_ = AttributeType::String;
						header.array_.size_ = arr.size();
						header.array_.add_ = [&obj]() { obj.targetTags_.push_back({}); };
						header.array_.remove_ = [&obj](Size idx) { if (idx < obj.targetTags_.size()) obj.targetTags_.erase(obj.targetTags_.begin() + idx); };
						outInfo.push_back(std::move(header));
						for (Size i = 0; i < arr.size(); ++i)
						{
							outInfo.push_back({ String("[" + std::to_string(i) + "]"), 0, AttributeType::String, PayloadType::None, &arr[i] });
						}
					}
					{
						FieldInfo fi;
						fi.name_ = String("クールタイム");
						fi.offset_ = offsetof(SwitchSystem, cooltime_);
						fi.type_ = AttributeType::Float;
						fi.clampMin_ = 0.0f;
						fi.clampMax_ = 10000.0f;
						outInfo.push_back(std::move(fi));
					}
					outInfo.push_back({ String("判定範囲を描画するか"), offsetof(SwitchSystem, isDebugAreaDraw_), AttributeType::Bool });
				});
			}
		};
		static Register_SwitchSystem global_SwitchSystem_register;

		// ---- UserProject/Script/TitleLogoController.h ----
		struct Register_TitleLogoController
		{
			Register_TitleLogoController()
			{
				ReflectionRegistry::Register(String("TitleLogoController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					TitleLogoController& obj = *static_cast<TitleLogoController*>(ptr);
					outInfo.push_back({ String("moveTime"), offsetof(TitleLogoController, moveTime), AttributeType::Float });
					outInfo.push_back({ String("minPosY"), offsetof(TitleLogoController, minPosY), AttributeType::Float });
					outInfo.push_back({ String("maxPosY"), offsetof(TitleLogoController, maxPosY), AttributeType::Float });
				});
			}
		};
		static Register_TitleLogoController global_TitleLogoController_register;

		struct RegisterEnum_CircuitSystem_Logic
		{
			RegisterEnum_CircuitSystem_Logic()
			{
				EnumRegistry::Register(String("CircuitSystem::Logic"), {
					{ static_cast<Int>(CircuitSystem::Logic::And), String("And") },
					{ static_cast<Int>(CircuitSystem::Logic::Or), String("Or") },
					{ static_cast<Int>(CircuitSystem::Logic::Count), String("Count") },
				});
			}
		};
		static RegisterEnum_CircuitSystem_Logic global_CircuitSystem_Logic_enum_register;

		struct RegisterEnum_FloorMoveSystem_MoveType
		{
			RegisterEnum_FloorMoveSystem_MoveType()
			{
				EnumRegistry::Register(String("FloorMoveSystem::MoveType"), {
					{ static_cast<Int>(FloorMoveSystem::MoveType::PingPong), String("PingPong") },
					{ static_cast<Int>(FloorMoveSystem::MoveType::Repeat), String("Repeat") },
				});
			}
		};
		static RegisterEnum_FloorMoveSystem_MoveType global_FloorMoveSystem_MoveType_enum_register;

		struct RegisterEnum_SwitchSystem_Mode
		{
			RegisterEnum_SwitchSystem_Mode()
			{
				EnumRegistry::Register(String("SwitchSystem::Mode"), {
					{ static_cast<Int>(SwitchSystem::Mode::Momentary), String("Momentary") },
					{ static_cast<Int>(SwitchSystem::Mode::Toggle), String("Toggle") },
					{ static_cast<Int>(SwitchSystem::Mode::Latch), String("Latch") },
				});
			}
		};
		static RegisterEnum_SwitchSystem_Mode global_SwitchSystem_Mode_enum_register;

		struct RegisterEnum_SwitchSystem_Shape
		{
			RegisterEnum_SwitchSystem_Shape()
			{
				EnumRegistry::Register(String("SwitchSystem::Shape"), {
					{ static_cast<Int>(SwitchSystem::Shape::Sphere), String("Sphere") },
					{ static_cast<Int>(SwitchSystem::Shape::Box), String("Box") },
				});
			}
		};
		static RegisterEnum_SwitchSystem_Shape global_SwitchSystem_Shape_enum_register;

	}
}