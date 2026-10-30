#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Payload/PayloadRegistry.h>
#include <UserProject/Script/AreaController.h>
#include <UserProject/Script/BallReseterController.h>
#include <UserProject/Script/CircuitSystem.h>
#include <UserProject/Script/CostGaugeController.h>
#include <UserProject/Script/DoorSystem.h>
#include <UserProject/Script/FloorMoveSystem.h>
#include <UserProject/Script/GoalSystem.h>

extern "C" int _force_payload_AreaController = 0;
extern "C" int _force_payload_BallReseterController = 0;
extern "C" int _force_payload_CircuitSystem = 0;
extern "C" int _force_payload_CostGaugeController = 0;
extern "C" int _force_payload_DoorSystem = 0;
extern "C" int _force_payload_FloorMoveSystem = 0;
extern "C" int _force_payload_GoalSystem = 0;

namespace SeedCore
{
	 namespace ScPayload
	 {
		// ---- UserProject/Script/AreaController.h ----
		struct Register_AreaController
		{
			Register_AreaController()
			{
				PayloadRegistry::Register(String("AreaController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					AreaController& obj = *static_cast<AreaController*>(ptr);
					outInfo.push_back({ String("lerpMaterial"), offsetof(AreaController, lerpMaterial), AttributeType::Int, PayloadType::Material });
				});
			}
		};
		static Register_AreaController global_AreaController_register;

		// ---- UserProject/Script/BallReseterController.h ----
		struct Register_BallReseterController
		{
			Register_BallReseterController()
			{
				PayloadRegistry::Register(String("BallReseterController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					BallReseterController& obj = *static_cast<BallReseterController*>(ptr);
					outInfo.push_back({ String("対象ボール"), offsetof(BallReseterController, ballActorID), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_BallReseterController global_BallReseterController_register;

		// ---- UserProject/Script/CircuitSystem.h ----
		struct Register_CircuitSystem
		{
			Register_CircuitSystem()
			{
				PayloadRegistry::Register(String("CircuitSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CircuitSystem& obj = *static_cast<CircuitSystem*>(ptr);
					{
						auto& arr = obj.inputActorIDs_;
						FieldInfo header;
						header.name_ = String("入力アクター");
						header.offset_ = 0;
						header.type_ = AttributeType::Int;
						header.assetType_ = PayloadType::Actor;
						header.array_.size_ = arr.size();
						header.array_.add_ = [&obj]() { obj.inputActorIDs_.push_back({}); };
						header.array_.remove_ = [&obj](Size idx) { if (idx < obj.inputActorIDs_.size()) obj.inputActorIDs_.erase(obj.inputActorIDs_.begin() + idx); };
						header.array_.lastPtr_ = [&obj]() -> void* { return &obj.inputActorIDs_.back(); };
						outInfo.push_back(std::move(header));
						for (Size i = 0; i < arr.size(); ++i)
						{
							outInfo.push_back({ String("[" + std::to_string(i) + "]"), 0, AttributeType::Int, PayloadType::Actor, &arr[i] });
						}
					}
				});
			}
		};
		static Register_CircuitSystem global_CircuitSystem_register;

		// ---- UserProject/Script/CostGaugeController.h ----
		struct Register_CostGaugeController
		{
			Register_CostGaugeController()
			{
				PayloadRegistry::Register(String("CostGaugeController"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CostGaugeController& obj = *static_cast<CostGaugeController*>(ptr);
					outInfo.push_back({ String("costGaugeAddSprite"), offsetof(CostGaugeController, costGaugeAddSprite), AttributeType::Int, PayloadType::Texture });
					outInfo.push_back({ String("costGaugeOverSprite"), offsetof(CostGaugeController, costGaugeOverSprite), AttributeType::Int, PayloadType::Texture });
				});
			}
		};
		static Register_CostGaugeController global_CostGaugeController_register;

		// ---- UserProject/Script/DoorSystem.h ----
		struct Register_DoorSystem
		{
			Register_DoorSystem()
			{
				PayloadRegistry::Register(String("DoorSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					DoorSystem& obj = *static_cast<DoorSystem*>(ptr);
					outInfo.push_back({ String("起動元回路"), offsetof(DoorSystem, circuitActorID_), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_DoorSystem global_DoorSystem_register;

		// ---- UserProject/Script/FloorMoveSystem.h ----
		struct Register_FloorMoveSystem
		{
			Register_FloorMoveSystem()
			{
				PayloadRegistry::Register(String("FloorMoveSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					FloorMoveSystem& obj = *static_cast<FloorMoveSystem*>(ptr);
					{
						FieldInfo fi;
						fi.name_ = String("起動元回路");
						fi.offset_ = offsetof(FloorMoveSystem, circuitActorID_);
						fi.type_ = AttributeType::Int;
						fi.assetType_ = PayloadType::Actor;
						fi.enableIf_ = [](void* p) -> Bool { auto& o = *static_cast<FloorMoveSystem*>(p); return o.isCanCircuit_; };
						outInfo.push_back(std::move(fi));
					}
				});
			}
		};
		static Register_FloorMoveSystem global_FloorMoveSystem_register;

		// ---- UserProject/Script/GoalSystem.h ----
		struct Register_GoalSystem
		{
			Register_GoalSystem()
			{
				PayloadRegistry::Register(String("GoalSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					GoalSystem& obj = *static_cast<GoalSystem*>(ptr);
					outInfo.push_back({ String("遷移先シーン"), offsetof(GoalSystem, nextSceneID_), AttributeType::Int, PayloadType::Scene });
					outInfo.push_back({ String("タイトルシーン"), offsetof(GoalSystem, titleSceneID_), AttributeType::Int, PayloadType::Scene });
					outInfo.push_back({ String("リトライ先シーン"), offsetof(GoalSystem, retrySceneID_), AttributeType::Int, PayloadType::Scene });
				});
			}
		};
		static Register_GoalSystem global_GoalSystem_register;

	}
}