#include <GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomStage.h>

namespace SeedCore
{
	void KawaseBloomFeature::PrefilterStage::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.prefilterPipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 2), Max<Uint32>(1, view->displayHeight_ / 2));
	}

	void KawaseBloomFeature::Downsample1Stage::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.downsample1Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void KawaseBloomFeature::Downsample2Stage::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.downsample2Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 8), Max<Uint32>(1, view->displayHeight_ / 8));
	}

	void KawaseBloomFeature::Downsample3Stage::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.downsample3Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 16), Max<Uint32>(1, view->displayHeight_ / 16));
	}

	void KawaseBloomFeature::Downsample4Stage::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.downsample4Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 32), Max<Uint32>(1, view->displayHeight_ / 32));
	}

	void KawaseBloomFeature::Downsample5Stage::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.downsample5Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 64), Max<Uint32>(1, view->displayHeight_ / 64));
	}

	void KawaseBloomFeature::Upsample4Stage::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.upsample4Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 32), Max<Uint32>(1, view->displayHeight_ / 32));
	}

	void KawaseBloomFeature::Upsample3Stage::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.upsample3Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 16), Max<Uint32>(1, view->displayHeight_ / 16));
	}

	void KawaseBloomFeature::Upsample2Stage::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.upsample2Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 8), Max<Uint32>(1, view->displayHeight_ / 8));
	}

	void KawaseBloomFeature::Upsample1Stage::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.upsample1Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 4), Max<Uint32>(1, view->displayHeight_ / 4));
	}

	void KawaseBloomFeature::Upsample0Stage::Execute(KawaseBloomFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		feature.upsample0Pipeline_.Dispatch(pass, Max<Uint32>(1, view->displayWidth_ / 2), Max<Uint32>(1, view->displayHeight_ / 2));
	}
}
