#include <GraphicsEngine/Shape/HUD/HUDStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void HUDFeature::ComposeStage::Execute(HUDFeature& feature, RenderPass& pass)
	{
		feature.composePipeline_.Draw(pass);
	}
}
