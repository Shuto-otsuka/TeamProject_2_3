#include <GraphicsEngine/Illumination/SubsurfaceScattering/Raytracing/RaytracingSubsurfaceScatteringStage.h>

namespace SeedCore
{
	void RaytracingSubsurfaceScatteringFeature::TraceStage::Execute(RaytracingSubsurfaceScatteringFeature& feature, RenderPass& pass)
	{
		feature.tracePipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}
}
