#pragma once
#include <GraphicsEngine/Shape/Primitive/PrimitiveShader.h>
#include <GraphicsEngine/Shape/Primitive/PrimitiveMesh.h>

namespace SeedCore
{
	class RenderFrame;
	class TextureStreaming;

	class PrimitiveSolid :public NonTransferable
	{
	public:
		SC_CONSTEX Uint32 capacity_ = 16384;

	public:
		PrimitiveSolid();

		~PrimitiveSolid();

		void Create(ID3D12Device* device, BindlessHeap& heap);

		void Gather(const ShapeList& list, TextureStreaming& textureStreaming, RenderFrame& frame);

	public:
		NODISCARD std::span<const PrimitiveSolidUpdate> Updates()const;

		NODISCARD std::span<const PrimitiveMaterialUpdate> Materials()const;

		NODISCARD const PrimitiveSolidConstantBuffer& Constants()const;

		NODISCARD Uint32 Count()const;

	private:
		struct Slot
		{
			Bool sent_ = false;

			PrimitiveSolidStructuredBuffer solid_;

			ModelMaterialStructuredBuffer material_{};
		};

	private:
		PrimitiveMesh mesh_;

		DynamicArray<Slot> slots_;

		DynamicArray<PrimitiveSolidUpdate> updates_;

		DynamicArray<PrimitiveMaterialUpdate> materials_;

		PrimitiveSolidConstantBuffer constants_;

		Uint32 count_ = 0;

		Uint64 frame_ = 0;
	};
}
