#include <GraphicsEngine/Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusionStage.h>

namespace SeedCore
{
	void RaytracingAmbientOcclusionFeature::TraceStage::Execute(RaytracingAmbientOcclusionFeature& feature, RenderPass& pass)
	{
		feature.tracePipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}

	void RaytracingAmbientOcclusionFeature::FilterStage::Execute(RaytracingAmbientOcclusionFeature& feature, RenderPass& pass)
	{
		feature.temporalPipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}
}
