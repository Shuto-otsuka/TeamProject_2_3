#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct CapsuleShape
	{
		SC_PAYLOAD_FIELD_EX("テクスチャID", Texture)
		Uint32 textureID_ = 0;

		SC_REFLECTION_FIELD_CONDITION(textureID_ != 0)
		SC_REFLECTION_FIELD_EX("UVの繰り返し")
		Vector2 uvScale_ = { 1.0f,1.0f };

		SC_REFLECTION_FIELD_CONDITION(textureID_ != 0)
		SC_REFLECTION_FIELD_EX("UVのずらし")
		Vector2 uvOffset_ = { 0.0f,0.0f };

		SC_REFLECTION_FIELD_EX("色")
		Color color_ = { 1.0f,1.0f,1.0f,1.0f };

		SC_REFLECTION_FIELD_EX("高さ")
		Float height_ = 1.0f;

		SC_REFLECTION_FIELD_EX("半径")
		Float radius_ = 0.5f;
	};
	REGISTER_COMPONENT(CapsuleShape, "Geometry", ComponentStorage::Archetype);
}