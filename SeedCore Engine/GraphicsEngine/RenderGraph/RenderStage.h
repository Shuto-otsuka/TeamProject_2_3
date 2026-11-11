#pragma once
#include <GraphicsEngine/RenderGraph/RenderAccess.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>

namespace SeedCore
{
	class RenderPass;

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
		Occlusion = 1100,
		Illumination = 1500,
		Lighting = 2000,
		Compose = 2500,
		Background = 2600,
		Transparent = 3000,
		PostProcess = 4000,
		Overlay = 5000,
	};

	struct RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Compute;

		SC_CONSTEX RenderViewMask viewMask_ = RenderViewMask::Scene;

		SC_CONSTEX Bool async_ = false;

		SC_CONSTEX std::nullptr_t enableCondition_ = nullptr;
	};
}
