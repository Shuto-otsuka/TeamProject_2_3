#pragma once
#include <FoundationEngine/Log/Assert.h>

namespace SeedCore
{
	/**
	* [EN]
	* What a Texture.Units row draws. Mirrors TEXTURE_KIND_* in
	* Texture/Texture.hlsli.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Texture.Units の 1 行が描くもの。Texture/Texture.hlsli の
	* TEXTURE_KIND_* と対応する。
	*/
	enum class TextureKind :Uint32
	{
		Texture,
		Font,
		Movie,
	};

	/**
	* [EN]
	* Where a Texture.Units row is drawn. Mirrors TEXTURE_DISPLAY_* in
	* Texture/Texture.hlsli.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Texture.Units の 1 行を描く場所。Texture/Texture.hlsli の
	* TEXTURE_DISPLAY_* と対応する。
	*/
	enum class TextureDisplay :Uint32
	{
		Sprite,
		Billboard,
		Fullscreen,
	};

	/**
	* [EN]
	* One actor's component (Image, Text or Movie) - see Texture.hlsli's
	* TextureUnitStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アクターのコンポーネント（Image・Text・Movie）1 つ分。Texture.hlsli の
	* TextureUnitStructuredBuffer を参照。
	*/
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

	/**
	* [EN]
	* One quad of a unit - see Texture.hlsli's TextureQuadStructuredBuffer.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ユニットの四角形 1 つ分。Texture.hlsli の
	* TextureQuadStructuredBuffer を参照。
	*/
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

	/**
	* [EN]
	* A changed Texture.Units row, written to the table by Texture.Update.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 変わった Texture.Units の行。Texture.Update が表へ書く。
	*/
	struct TextureUnitUpdate
	{
		Uint index_ = 0;
		Uint textureUnitUpdatePadding0_[3] = { 0, 0, 0 };

		TextureUnitStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(TextureUnitUpdate, 208, "Texture/Texture.hlsli");

	/**
	* [EN]
	* A changed Texture.Quads row, written to the table by Texture.Update.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 変わった Texture.Quads の行。Texture.Update が表へ書く。
	*/
	struct TextureQuadUpdate
	{
		Uint index_ = 0;
		Uint textureQuadUpdatePadding0_[3] = { 0, 0, 0 };

		TextureQuadStructuredBuffer data_;
	};
	SC_STATIC_ASSERT(TextureQuadUpdate, 64, "Texture/Texture.hlsli");

	/**
	* [EN]
	* Texture.Constants: where the sprite quads (screen quads, drawn first) and
	* the billboard quads start in Texture.DrawList and how many of each there
	* are, and the quad slots Texture.Expand walks.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Texture.Constants。Texture.DrawList の中の、スプライト（画面の四角形。
	* 先に描く）とビルボードの四角形それぞれの先頭と数、Texture.Expand が回す
	* 四角形の枠の数。
	*/
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
