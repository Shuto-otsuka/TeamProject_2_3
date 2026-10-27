#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	enum class CausticsTechnique
	{
		Raytracing,
		ScreenSpace,
	};

	struct RaytracingCausticsSettings
	{
		
	};

	struct ScreenSpaceCausticsSettings
	{

	};

	struct Caustics
	{
		SC_REFLECTION_FIELD_EX("有効")
		Bool enable_ = true;

		SC_REFLECTION_FIELD_CONDITION(enable_)
		SC_REFLECTION_FIELD_EX("手法")
		CausticsTechnique technique_ = CausticsTechnique::Raytracing;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == CausticsTechnique::Raytracing)
		SC_REFLECTION_FIELD_EX("レイトレーシングコースティクス")
		RaytracingCausticsSettings raytracing_;

		SC_REFLECTION_FIELD_CONDITION(enable_&& technique_ == CausticsTechnique::ScreenSpace)
		SC_REFLECTION_FIELD_EX("スクリーンスペースコースティクス")
		ScreenSpaceCausticsSettings screenSpace_;
	};
	REGISTER_COMPONENT(Caustics, "Graphics/Illumination");
}