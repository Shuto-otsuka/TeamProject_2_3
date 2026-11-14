#include <GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLightStage.h>

namespace SeedCore
{
	void RaytracingVolumetricLightFeature::BuildStage::Execute(RaytracingVolumetricLightFeature& feature, RenderPass& pass)
	{
		feature.injectPipeline_.Dispatch(pass, (pass.Width() + froxelDivisor_ - 1) / froxelDivisor_, (pass.Height() + froxelDivisor_ - 1) / froxelDivisor_, froxelDepth_);
	}

	void RaytracingVolumetricLightFeature::TraceStage::Execute(RaytracingVolumetricLightFeature& feature, RenderPass& pass)
	{
		feature.scatterPipeline_.Dispatch(pass, (pass.Width() + froxelDivisor_ - 1) / froxelDivisor_, (pass.Height() + froxelDivisor_ - 1) / froxelDivisor_, froxelDepth_);
	}

	void RaytracingVolumetricLightFeature::ResolveStage::Execute(RaytracingVolumetricLightFeature& feature, RenderPass& pass)
	{
		feature.integratePipeline_.Dispatch(pass, (pass.Width() + froxelDivisor_ - 1) / froxelDivisor_, (pass.Height() + froxelDivisor_ - 1) / froxelDivisor_, 1, feature.tracing_ ? 1 : 0);
	}
}
