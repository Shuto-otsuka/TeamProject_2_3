#include <GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareStage.h>

namespace SeedCore
{
	void AnamorphicFlareFeature::PrefilterStage::Execute(AnamorphicFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.prefilterPipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 8), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void AnamorphicFlareFeature::Blur1Stage::Execute(AnamorphicFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.blur1Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 8), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void AnamorphicFlareFeature::Blur2Stage::Execute(AnamorphicFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.blur2Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 8), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void AnamorphicFlareFeature::Blur3Stage::Execute(AnamorphicFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.blur3Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 8), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void AnamorphicFlareFeature::Blur4Stage::Execute(AnamorphicFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.blur4Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 8), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void AnamorphicFlareFeature::ComposeStage::Execute(AnamorphicFlareFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.composePipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}
}
