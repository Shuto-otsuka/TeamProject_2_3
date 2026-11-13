#pragma once
#include <GraphicsEngine/PostProcess/FilmGrain/FilmGrainFeature.h>
#include <GraphicsEngine/PostProcess/Sharpness/SharpnessFeature.h>
#include <GraphicsEngine/Shape/HUD/HUDFeature.h>

namespace SeedCore
{
	struct FilmGrainFeature::ResolveStage :RenderStage
	{
		SC_CONSTEX RenderQueue queue_ = RenderQueue::PostProcess;

		SC_CONSTEX Bool FilmGrainFeature::* enableCondition_ = &FilmGrainFeature::enabled_;

		SC_CONSTEX Uint64 showFlags_ = static_cast<Uint64>(RenderShowFlag::PostProcess) | static_cast<Uint64>(RenderShowFlag::FilmGrain);

		SC_CONSTEX RenderAccess accesses_[] =
		{
			RenderAccess::Read(SharpnessFeature::Color),
			RenderAccess::Constant(Constants),
			RenderAccess::Write(HUDFeature::DisplayColor)
		};

		static void Execute(FilmGrainFeature& feature, RenderPass& pass);
	};
}
