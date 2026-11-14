#pragma once
#include <GraphicsEngine/PostProcess/PostProcessFeature.h>
#include <GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareFeature.h>
#include <GraphicsEngine/PostProcess/AutoExposure/AutoExposureFeature.h>
#include <GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomFeature.h>
#include <GraphicsEngine/PostProcess/LensFlare/LensFlareFeature.h>
#include <GraphicsEngine/PostProcess/Vignette/VignetteFeature.h>

namespace SeedCore
{
	struct PostProcessFeature::ComposeStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(VignetteFeature::Color),
			RenderAccess::Read(KawaseBloomFeature::Radiance),
			RenderAccess::Read(LensFlareFeature::Radiance),
			RenderAccess::Read(AnamorphicFlareFeature::Radiance),
			RenderAccess::Read(AutoExposureFeature::Exposure),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(Color)
		};

		static void Execute(PostProcessFeature& feature, RenderPass& pass);
	};
}
