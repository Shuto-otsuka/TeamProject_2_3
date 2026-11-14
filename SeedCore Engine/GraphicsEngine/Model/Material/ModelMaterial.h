#pragma once
#include <GraphicsEngine/Model/ModelDriven.h>

namespace SeedCore
{
	struct Surface;

	class Crister;
	class TextureStreaming;

	class ModelMaterial :public NonTransferable
	{
	public:
		SC_CONSTEX Uint32 capacity_ = 32768;

	public:
		ModelMaterial();

		~ModelMaterial();

		void Begin(Uint64 frame);

		NODISCARD Uint32 Write(Uint32 slot, Bool reset, std::span<const Surface> surfaces, Uint32 surfaceCount, Crister& crister, TextureStreaming& textureStreaming);

		void Release(Uint32 slot);

	public:
		NODISCARD std::span<const ModelMaterialStructuredBuffer> Materials(Uint32 slot)const;

		NODISCARD std::span<const ModelMaterialUpdate> Updates()const;

	private:
		NODISCARD static ModelMaterialStructuredBuffer Build(const Surface& surface, Crister& crister, TextureStreaming& textureStreaming, Uint64 frame);

	private:
		struct Slot
		{
			Uint32 offset_ = SC_INVALID;

			Uint32 count_ = 0;

			DynamicArray<ModelMaterialStructuredBuffer> materials_;
		};

	private:
		Uint64 frame_ = 0;

		DynamicArray<Slot> slots_;

		DynamicArray<std::pair<Uint32, Uint32>> freeRanges_;

		Uint32 used_ = 0;

		Bool capacityLogged_ = false;

		DynamicArray<ModelMaterialUpdate> updates_;
	};
}
