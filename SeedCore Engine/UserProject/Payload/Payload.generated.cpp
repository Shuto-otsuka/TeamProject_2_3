#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Payload/PayloadRegistry.h>
#include <UserProject/Script/CircuitSystem.h>
#include <UserProject/Script/CircuitTestSystem.h>
#include <UserProject/Script/CostGaugeController.h>
#include <UserProject/Script/DoorSystem.h>
#include <UserProject/Script/GoalSystem.h>

extern "C" int _force_payload_CircuitSystem = 0;
extern "C" int _force_payload_CircuitTestSystem = 0;
extern "C" int _force_payload_CostGaugeController = 0;
extern "C" int _force_payload_DoorSystem = 0;
extern "C" int _force_payload_GoalSystem = 0;

namespace SeedCore
{
	 namespace ScPayload
	 {
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

		// ---- UserProject/Script/CircuitTestSystem.h ----
		struct Register_CircuitTestSystem
		{
			Register_CircuitTestSystem()
			{
				PayloadRegistry::Register(String("CircuitTestSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					CircuitTestSystem& obj = *static_cast<CircuitTestSystem*>(ptr);
					outInfo.push_back({ String("確認する回路"), offsetof(CircuitTestSystem, circuitActorID_), AttributeType::Int, PayloadType::Actor });
				});
			}
		};
		static Register_CircuitTestSystem global_CircuitTestSystem_register;

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