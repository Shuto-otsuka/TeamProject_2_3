#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	struct PlaneShape
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

		SC_REFLECTION_CLAMPED_EX("サイズ", 0.001f, 10000.0f)
		Vector2 size_ = { 1.0f,1.0f };

		SC_REFLECTION_FIELD_EX("両面表示")
		Bool doubleSided_ = false;
	};
	REGISTER_COMPONENT(PlaneShape, "Geometry", ComponentStorage::Archetype);
}