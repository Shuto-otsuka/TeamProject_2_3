#pragma once
#include <FoundationEngine/SeedScript.h>
#include <FoundationEngine/Utility/Handle.h>
#include <GraphicsEngine/Model/Crister.h>

namespace SeedCore
{
	class MaterialResource;

	class SEEDCORE_API Material :public SeedScript
	{
	private:
		friend class MaterialSystem;
		friend class ModelRenderer;
		friend class RaytracingRenderer;

	public:
		SC_PAYLOAD_FIELD_EX("マテリアル", Material)
		DynamicArray<Uint32> materialIDs_;

		SC_SERIALIZE_FIELD()
		DynamicArray<Uint32> slotMaterialIDs_;

		SC_SERIALIZE_FIELD()
		DynamicArray<String> slotNames_;

	public:
		void OnInspectorGUI();

	public:
		void Set(Uint32 materialID, String slotName);

		void Set(Uint32 materialID, String slotName, Float lerpFactor);

	private:
		void Apply(const Crister& crister, const Handle<Crister>& cristerHandle, MaterialResource& materialResource);

	private:
		DynamicArray<String> lerpSlotNames_;

		DynamicArray<Uint32> lerpFromIDs_;

		DynamicArray<Float> lerpFactors_;

		DynamicArray<Surface> resolvedSurfaces_;

		Uint64 resolvedSignature_ = 0;
	};
	REGISTER_COMPONENT(Material, "Geometry");
}
