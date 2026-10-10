#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Payload/PayloadRegistry.h>
#include <UserProject/Script/CostGaugeController.h>

extern "C" int _force_payload_CostGaugeController = 0;

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

	}
}