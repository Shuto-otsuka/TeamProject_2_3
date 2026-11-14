#include <GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationStage.h>

namespace SeedCore
{
	void RaytracingGlobalIlluminationFeature::TraceStage::Execute(RaytracingGlobalIlluminationFeature& feature, RenderPass& pass)
	{
		feature.tracePipeline_.DispatchRays(pass, pass.Width(), pass.Height());
	}

	void RaytracingGlobalIlluminationFeature::FilterStage::SpatialStep::Execute(RaytracingGlobalIlluminationFeature& feature, RenderPass& pass)
	{
		feature.spatialPipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}

	void RaytracingGlobalIlluminationFeature::FilterStage::TemporalStep::Execute(RaytracingGlobalIlluminationFeature& feature, RenderPass& pass)
	{
		feature.temporalPipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}

	void RaytracingGlobalIlluminationFeature::FilterStage::ATrousStep::Execute(RaytracingGlobalIlluminationFeature& feature, RenderPass& pass)
	{
		feature.aTrousPipeline_.Dispatch(pass, pass.Width(), pass.Height(), 1, pass.Iteration());
	}
}
