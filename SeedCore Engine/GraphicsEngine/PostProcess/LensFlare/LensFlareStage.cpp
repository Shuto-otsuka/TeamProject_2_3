#include <GraphicsEngine/PostProcess/LensFlare/LensFlareStage.h>

namespace SeedCore
{
	void LensFlareFeature::FilterStage::DownsampleStep::Execute(LensFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.downsamplePipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void LensFlareFeature::FilterStage::BlurStep::Execute(LensFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.blurPipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4), 1, pass.Iteration());
	}

	void LensFlareFeature::ComposeStage::GlareStep::Execute(LensFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.glarePipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void LensFlareFeature::ComposeStage::GhostStep::Execute(LensFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.ghostPipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}
}
