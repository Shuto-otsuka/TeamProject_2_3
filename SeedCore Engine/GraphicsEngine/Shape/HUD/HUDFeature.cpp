#include <GraphicsEngine/Shape/HUD/HUDFeature.h>
#include <GraphicsEngine/Shape/HUD/HUDStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(HUDFeature, OutputStage);

	void HUDFeature::Create(RenderDevice& device)
	{
		outputPipeline_.Create(device);
	}
}
