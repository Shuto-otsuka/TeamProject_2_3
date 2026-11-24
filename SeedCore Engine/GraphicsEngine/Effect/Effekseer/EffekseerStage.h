#pragma once
#include <GraphicsEngine/Effect/Effekseer/EffekseerFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Illumination/Radiance/RadianceFeature.h>

namespace SeedCore
{
	struct EffekseerFeature::DrawStage :RenderStage
	{
		SC_CONSTEX RenderStageKind kind_ = RenderStageKind::Callback;

		SC_CONSTEX RenderQueue queue_ = RenderQueue::Effect;

		SC_CONSTEX Bool EffekseerFeature::* enableCondition_ = &EffekseerFeature::active_;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Target(RadianceFeature::Color),
			RenderAccess::DepthTest(GeometryFeature::Depth)
		};

		static void Execute(EffekseerFeature& feature, RenderPass& pass);
	};
}