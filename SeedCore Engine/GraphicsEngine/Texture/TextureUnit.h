#pragma once
#include <GraphicsEngine/Texture/TextureDriven.h>

namespace SeedCore
{
	struct RenderScene;

	class TextureUnit :public NonTransferable
	{
	public:
		SC_CONSTEX Uint32 capacity_ = 16384;

		SC_CONSTEX Uint32 unitCapacity_ = capacity_ * 3;

		SC_CONSTEX Uint32 quadCapacity_ = 131072;

	public:
		TextureUnit();

		~TextureUnit();

		void Gather(const RenderScene& scene);

	public:
		NODISCARD std::span<const TextureUnitUpdate> UnitUpdates()const;

		NODISCARD std::span<const TextureQuadUpdate> QuadUpdates()const;

		NODISCARD const TextureConstantBuffer& Constants()const;

	private:
		struct Content
		{
			TextureDisplay display_ = TextureDisplay::Sprite;
			Uint32 textureIndex_ = SC_INVALID;
			Vector2 textureSize_ = { 0.0f, 0.0f };
			Vector2 textureDimension_ = { 0.0f, 0.0f };
			Vector2 texturePosition_ = { 0.0f, 0.0f };
			Vector2 pivot_ = { 0.0f, 0.0f };
			Float fontSize_ = 0.0f;
			Float letterSpacing_ = 0.0f;
			Float lineSpacing_ = 0.0f;
			Uint32 shadow_ = 0;
			Vector2 shadowOffset_ = { 0.0f, 0.0f };
		};

		struct Slot
		{
			Bool seen_ = false;
			Bool sent_ = false;
			Uint32 quadOffset_ = 0;
			Uint32 quadCount_ = 0;
			Uint32 quadCapacity_ = 0;
			Content content_;
			String text_;
			TextureUnitStructuredBuffer unit_;
			TextureUnitStructuredBuffer sentUnit_;
		};

		struct Drawn
		{
			Uint32 group_ = 0;
			Int order_ = 0;
			Uint32 slot_ = 0;
		};

	private:
		void WriteQuads(Uint32 slotIndex);

	private:
		DynamicArray<Slot> slots_;

		DynamicArray<std::pair<Uint32, Uint32>> freeRanges_;

		Uint32 quadEnd_ = 0;

		DynamicArray<TextureQuadStructuredBuffer> quads_;

		DynamicArray<Drawn> drawn_;

		DynamicArray<Uint32> selected_;

		DynamicArray<Uint8> selectedFlags_;

		DynamicArray<TextureUnitUpdate> unitUpdates_;

		DynamicArray<TextureQuadUpdate> quadUpdates_;

		TextureConstantBuffer constants_;

		Uint64 streamingFrame_ = 0;
	};
}
