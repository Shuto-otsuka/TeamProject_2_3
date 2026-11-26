#pragma once
#include <FoundationEngine/Log/Assert.h>

namespace SeedCore
{
	enum class TextureKind :Uint32
	{
		Texture,
		Font,
		Movie,
	};

	enum class TextureDisplay :Uint32
	{
		Sprite,
		Billboard,
		Fullscreen,
	};

	struct TextureUnitStructuredBuffer
	{
		Matrix world_;

		Color color_ = { 1.0f, 1.0f, 1.0f, 1.0f };

		Color outlineColor_ = { 0.0f, 0.0f, 0.0f, 0.0f };

		Color glowColor_ = { 0.0f, 0.0f, 0.0f, 0.0f };

		Color shadowColor_ = { 0.0f, 0.0f, 0.0f, 0.0f };

		Vector2 unitRange_ = { 0.0f, 0.0f };
		Float outlineWidth_ = 0.0f;
		Float glowPower_ = 0.0f;

		Vector2 scrollDirection_ = { 0.0f, 0.0f };
		Float scrollSpeed_ = 0.0f;
		Uint textureIndex_ = SC_INVALID;

		TextureKind kind_ = TextureKind::Texture;
		TextureDisplay display_ = TextureDisplay::Sprite;
		Uint flags_ = 0;
		Uint quadOffset_ = 0;

		Uint quadCount_ = 0;
		Uint drawBase_ = 0;
		Uint actor_ = SC_INVALID;

		Uint textureUnitPadding0_ = 0;
	};
	SC_STATIC_ASSERT(TextureUnitStructuredBuffer, 192, "Texture/Texture.hlsli");

	struct TextureQuadStructuredBuffer
	{
		Vector2 rectMin_ = { 0.0f, 0.0f };
		Vector2 rectSize_ = { 0.0f, 0.0f };

		Vector2 uvMin_ = { 0.0f, 0.0f };
		Vector2 uvMax_ = { 1.0f, 1.0f };

		Uint unit_ = SC_INVALID;
		Uint flags_ = 0;
		Uint textureQuadPadding0_[2] = { 0, 0 };
	};
	SC_STATIC_ASSERT(TextureQuadStructuredBuffer, 48, "Texture/Texture.hlsli");

	struct TextureUnitUpdate
	{
		Uint index_ = 0;
		Uint textureUnitUpdatePadding0_[3] = { 0, 0, 0 };

		TextureUnitStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(TextureUnitUpdate, 208, "Texture/Texture.hlsli");

	struct TextureQuadUpdate
	{
		Uint index_ = 0;
		Uint textureQuadUpdatePadding0_[3] = { 0, 0, 0 };

		TextureQuadStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(TextureQuadUpdate, 64, "Texture/Texture.hlsli");

	struct TextureConstantBuffer
	{
		Uint spriteBase_ = 0;
		Uint spriteCount_ = 0;
		Uint billboardBase_ = 0;
		Uint billboardCount_ = 0;
		Uint quadCount_ = 0;
		StaticArray<Uint, 3> padding_ = { 0, 0, 0 };
	};
	SC_STATIC_ASSERT(TextureConstantBuffer, 32, "Texture/Texture.hlsli");
}
