#include <GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadowStage.h>

namespace SeedCore
{
	void RaytracingShadowFeature::TraceStage::Execute(RaytracingShadowFeature& feature, RenderPass& pass)
	{
		feature.tracePipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}

	void RaytracingShadowFeature::FilterStage::TemporalStep::Execute(RaytracingShadowFeature& feature, RenderPass& pass)
	{
		feature.temporalPipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}

	void RaytracingShadowFeature::FilterStage::MomentsStep::Execute(RaytracingShadowFeature& feature, RenderPass& pass)
	{
		feature.momentsPipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}

	void RaytracingShadowFeature::FilterStage::ATrousStep::Execute(RaytracingShadowFeature& feature, RenderPass& pass)
	{
		feature.aTrousPipeline_.Dispatch(pass, pass.Width(), pass.Height(), 1, pass.Iteration());
	}
}
