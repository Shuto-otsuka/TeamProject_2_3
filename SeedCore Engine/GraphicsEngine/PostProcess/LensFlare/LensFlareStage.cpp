#include <GraphicsEngine/PostProcess/LensFlare/LensFlareStage.h>

namespace SeedCore
{
	void LensFlareFeature::DownsampleStage::Execute(LensFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.downsamplePipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void LensFlareFeature::BlurPass1Stage::Execute(LensFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.blurPass1Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void LensFlareFeature::BlurPass2Stage::Execute(LensFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.blurPass2Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void LensFlareFeature::BlurPass3Stage::Execute(LensFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.blurPass3Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void LensFlareFeature::BlurPass4Stage::Execute(LensFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.blurPass4Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void LensFlareFeature::ComposeStage::Execute(LensFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.composePipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void LensFlareFeature::GhostStage::Execute(LensFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.ghostPipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}
}
