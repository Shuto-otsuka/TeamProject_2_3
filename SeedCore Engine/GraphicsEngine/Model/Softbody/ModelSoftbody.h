#pragma once
#include <GraphicsEngine/Model/ModelDriven.h>

namespace SeedCore
{
	class Actor;
	class Crister;
	class RenderDevice;
	class SoftbodyMesh;

	class ModelSoftbody :public NonTransferable
	{
	public:
		ModelSoftbody();

		~ModelSoftbody();

		void Initialize(RenderDevice& device);

		void Begin();

		NODISCARD Bool Write(Uint32 slot, Bool reset, Actor& actor, Crister& crister, const Matrix& world, const Matrix& previousWorld, const ModelMaterialStructuredBuffer& material, Bool selected);

		void Release(Uint32 slot);

		void End();

	public:
		NODISCARD std::span<const ModelStructuredBuffer> Instances()const;

		NODISCARD Uint32 OpaqueCount()const;

		NODISCARD Uint32 TransparentCount()const;

	private:
		RenderDevice* device_ = nullptr;

		DynamicArray<ResourcePtr<SoftbodyMesh>> slots_;

		DynamicArray<ModelStructuredBuffer> opaque_;

		DynamicArray<ModelStructuredBuffer> transparent_;

		DynamicArray<ModelStructuredBuffer> instances_;
	};
}
