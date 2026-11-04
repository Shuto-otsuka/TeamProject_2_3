#pragma once
#include <GraphicsEngine/RenderGraph/RenderName.h>
#include <GraphicsEngine/RenderGraph/RenderAccess.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	class RenderStageContext;

	enum class RenderStageKind :Uint8
	{
		Raster,
		Compute,
		Raytracing,
		Build,
		Copy,
		Callback,
	};

	enum class RenderQueue :Uint32
	{
		Frame = 0,
		Geometry = 1000,
		Illumination = 1500,
		Lighting = 2000,
		Compose = 2500,
		Background = 2600,
		Transparent = 3000,
		PostProcess = 4000,
		Overlay = 5000,
	};

	template<class... Stages>
	struct RenderStageList
	{
		SC_CONSTEX StaticArray<std::string_view, sizeof...(Stages)> names_ = { Stages::name_.Name()... };

		SC_CONSTEX StaticArray<std::span<const RenderAccess>, sizeof...(Stages)> accesses_ = { std::span<const RenderAccess>(Stages::accesses_)... };
	};

	class RenderStageAccess
	{
	public:
		template<class Stage, class Feature>
		SC_CONSTEX Bool IsRenderStage = requires(Feature & feature, RenderStageContext & context)
		{
			{ Stage::Execute(feature, context) } -> std::same_as<void>;
			requires std::same_as<std::remove_cv_t<decltype(Stage::name_)>, RenderStageName>;
			requires std::same_as<std::remove_cv_t<decltype(Stage::kind_)>, RenderStageKind>;
			requires std::same_as<std::remove_cv_t<decltype(Stage::queue_)>, RenderQueue>;
			requires std::same_as<std::remove_cv_t<decltype(Stage::viewMask_)>, RenderViewMask>;
			requires std::same_as<std::remove_cv_t<decltype(Stage::enableCondition_)>, std::nullptr_t> || std::same_as<std::remove_cv_t<decltype(Stage::enableCondition_)>, Bool Feature::*> || std::same_as<std::remove_cv_t<decltype(Stage::enableCondition_)>, Bool(Feature::*)[RenderViewCount]>;
			requires std::is_array_v<decltype(Stage::accesses_)> && std::same_as<std::remove_cv_t<std::remove_extent_t<decltype(Stage::accesses_)>>, RenderAccess>;
		};
	};
}