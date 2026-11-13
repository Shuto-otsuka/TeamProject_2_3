#include <GraphicsEngine/Screen/Boot/BootStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void BootFeature::DrawStage::Execute(BootFeature& feature, RenderPass& pass)
	{
		feature.drawPipeline_.Draw(pass);
	}
}
