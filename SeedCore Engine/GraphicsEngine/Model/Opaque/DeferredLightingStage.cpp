#include <GraphicsEngine/Model/Opaque/DeferredLightingStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void DeferredLightingFeature::ResolveStage::Execute(DeferredLightingFeature& feature, RenderPass& pass)
	{
		feature.resolvePipeline_.Draw(pass);
	}
}
