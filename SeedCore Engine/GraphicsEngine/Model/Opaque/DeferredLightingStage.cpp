#include <GraphicsEngine/Model/Opaque/DeferredLightingStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void DeferredLightingFeature::DrawStage::Execute(DeferredLightingFeature& feature, RenderPass& pass)
	{
		feature.drawPipeline_.Draw(pass);
	}
}
