#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionStage.h>

namespace SeedCore
{
	void RaytracingReflectionFeature::TraceStage::Execute(RaytracingReflectionFeature& feature, RenderPass& pass)
	{
		feature.tracePipeline_.DispatchRays(pass, pass.Width(), pass.Height());
	}

	void RaytracingReflectionFeature::FilterStage::SpatialStep::Execute(RaytracingReflectionFeature& feature, RenderPass& pass)
	{
		feature.spatialPipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}

	void RaytracingReflectionFeature::FilterStage::TemporalStep::Execute(RaytracingReflectionFeature& feature, RenderPass& pass)
	{
		feature.temporalPipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}

	void RaytracingReflectionFeature::FilterStage::MomentsStep::Execute(RaytracingReflectionFeature& feature, RenderPass& pass)
	{
		feature.momentsPipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}

	void RaytracingReflectionFeature::FilterStage::ATrousStep::Execute(RaytracingReflectionFeature& feature, RenderPass& pass)
	{
		feature.aTrousPipeline_.Dispatch(pass, pass.Width(), pass.Height(), 1, pass.Iteration());
	}
}
