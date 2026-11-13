#include <GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomStage.h>

namespace SeedCore
{
	void KawaseBloomFeature::FilterStage::PrefilterStep::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.prefilterPipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 2), Max<Uint32>(1, view->displayHeight_ / 2));
	}

	void KawaseBloomFeature::FilterStage::DownsampleStep::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		Uint32 shift = pass.Iteration() + 2;
		feature.downsamplePipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ >> shift), Max<Uint32>(1, view->displayHeight_ >> shift), 1, pass.Iteration());
	}

	void KawaseBloomFeature::FilterStage::UpsampleStep::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		Uint32 shift = 5 - pass.Iteration();
		feature.upsamplePipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ >> shift), Max<Uint32>(1, view->displayHeight_ >> shift), 1, pass.Iteration());
	}
}
