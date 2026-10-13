#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	class SEEDCORE_API Material :public SeedScript
	{
	public:
		SC_PAYLOAD_FIELD_EX("マテリアル", Material)
		DynamicArray<Uint32> materialIDs_;

		SC_SERIALIZE_FIELD()
		DynamicArray<Uint32> slotMaterialIDs_;

		SC_SERIALIZE_FIELD()
		DynamicArray<String> slotNames_;

	public:
		void OnInspectorGUI();

	};
	REGISTER_COMPONENT(Material, "Geometry");
}
