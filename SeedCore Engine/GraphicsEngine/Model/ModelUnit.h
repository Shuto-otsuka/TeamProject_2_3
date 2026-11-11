#pragma once
#include <GraphicsEngine/Model/ModelDriven.h>

namespace SeedCore
{
	struct RenderScene;

	class Crister;
	class ModelMaterial;
	class ModelMesh;
	class ModelMorph;
	class ModelSkin;
	class ModelSoftbody;

	class ModelUnit :public NonTransferable
	{
	public:
		SC_CONSTEX Uint32 capacity_ = 131072;

		SC_CONSTEX Uint32 instanceCapacity_ = 131072;

		SC_CONSTEX Uint32 transparentBase_ = 98304;

		SC_CONSTEX Uint32 furBase_ = 122880;

	public:
		ModelUnit();

		~ModelUnit();

		void Gather(const RenderScene& scene, ModelMesh& meshes, ModelMaterial& materials, ModelSkin& skins, ModelMorph& morphs, ModelSoftbody& softbodies);

		void Finish(const RenderScene& scene, const ModelSoftbody& softbodies);

	public:
		NODISCARD std::span<const ModelUnitUpdate> Updates()const;

		NODISCARD const ModelExpandConstantBuffer& Constants()const;

		NODISCARD Uint32 Count()const;

	private:
		struct Slot
		{
			Bool used_ = false;

			Bool seen_ = false;

			Bool sent_ = false;

			Uint32 generation_ = 0;

			const Crister* crister_ = nullptr;

			Matrix previousWorld_;

			ModelUnitStructuredBuffer unit_;
		};

	private:
		DynamicArray<Slot> slots_;

		Uint32 count_ = 0;

		Uint32 initializedCount_ = 0;

		DynamicArray<Uint32> selected_;

		DynamicArray<Uint8> selectedFlags_;

		DynamicArray<ModelUnitUpdate> updates_;

		ModelExpandConstantBuffer constants_;
	};
}
