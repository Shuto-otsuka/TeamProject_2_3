#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Payload/PayloadRegistry.h>
#include <UserProject/Script/CostGaugeController.h>
#include <UserProject/Script/GoalSystem.h>

extern "C" int _force_payload_CostGaugeController = 0;
extern "C" int _force_payload_GoalSystem = 0;

namespace SeedCore
{
	 namespace ScPayload
	 {
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

		// ---- UserProject/Script/GoalSystem.h ----
		struct Register_GoalSystem
		{
			Register_GoalSystem()
			{
				PayloadRegistry::Register(String("GoalSystem"), [](void* ptr, DynamicArray<FieldInfo>& outInfo) {
					GoalSystem& obj = *static_cast<GoalSystem*>(ptr);
					outInfo.push_back({ String("遷移先シーン"), offsetof(GoalSystem, translationSceneID_), AttributeType::Int, PayloadType::Scene });
					outInfo.push_back({ String("タイトルシーン"), offsetof(GoalSystem, titleSceneID_), AttributeType::Int, PayloadType::Scene });
				});
			}
		};
		static Register_GoalSystem global_GoalSystem_register;

	}
}