#include <GraphicsEngine/Environment/VolumetricStar/VolumetricStarStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void VolumetricStarFeature::TraceStage::Execute(VolumetricStarFeature& feature, RenderPass& pass)
	{
		feature.tracePipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}
}
