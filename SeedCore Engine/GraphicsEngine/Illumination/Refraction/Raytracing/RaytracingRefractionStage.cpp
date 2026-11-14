#include <GraphicsEngine/Illumination/Refraction/Raytracing/RaytracingRefractionStage.h>

namespace SeedCore
{
	void RaytracingRefractionFeature::TraceStage::Execute(RaytracingRefractionFeature& feature, RenderPass& pass)
	{
		feature.tracePipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}
}
