#pragma once
#include <GraphicsEngine/Debug/DebugViewFeature.h>
#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Shape/HUD/HUDFeature.h>

namespace SeedCore
{
	struct DebugViewFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::Overlay;

		SC_CONSTEX Bool(DebugViewFeature::* enableCondition_)[RenderViewCount] = &DebugViewFeature::resolving_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(GeometryFeature::Depth),
			RenderAccess::Read(GeometryFeature::VisibilityId),
			RenderAccess::Read(MaterialFeature::SurfaceBaseColor),
			RenderAccess::Read(MaterialFeature::SurfaceNormalRoughness),
			RenderAccess::Read(MaterialFeature::Velocity),
			RenderAccess::Read(MaterialFeature::SurfaceRadiance),
			RenderAccess::Write(HUDFeature::DisplayColor)
		};

		static void Execute(DebugViewFeature& feature, RenderPass& pass);
	};
}
