#include <GraphicsEngine/PostProcess/FilmGrain/FilmGrainStage.h>

namespace SeedCore
{
	void FilmGrainFeature::ResolveStage::Execute(FilmGrainFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.resolvePipeline_.Dispatch(pass, view->displayWidth_, view->displayHeight_);
	}
}
