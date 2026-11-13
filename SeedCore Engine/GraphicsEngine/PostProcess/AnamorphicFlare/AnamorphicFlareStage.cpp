#include <GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareStage.h>

namespace SeedCore
{
	void AnamorphicFlareFeature::FilterStage::PrefilterStep::Execute(AnamorphicFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.prefilterPipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 8), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void AnamorphicFlareFeature::FilterStage::BlurStep::Execute(AnamorphicFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.blurPipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 8), Max<Uint32>(1, view->displayHeight_ / 4), 1, pass.Iteration());
	}

	void AnamorphicFlareFeature::ComposeStage::Execute(AnamorphicFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.composePipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}
}
