#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	enum class OutlineShape
	{
		Outer,
		Contour,
	};

	struct Outline
	{
		SC_REFLECTION_FIELD_EX("線の付き方")
		OutlineShape shape_ = OutlineShape::Outer;

		SC_REFLECTION_FIELD_EX("色")
		Color color_ = { 1.0f, 0.8f, 0.1f, 1.0f };

		SC_REFLECTION_CLAMPED_EX("幅", 0.0f, 8.0f)
		Float width_ = 2.0f;
	};
	REGISTER_COMPONENT(Outline, "PostProcess", ComponentStorage::SparseSet);
}
