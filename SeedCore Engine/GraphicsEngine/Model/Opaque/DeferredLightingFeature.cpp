#include <GraphicsEngine/Model/Opaque/DeferredLightingFeature.h>
#include <GraphicsEngine/Model/Opaque/DeferredLightingStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(DeferredLightingFeature, DrawStage);

	void DeferredLightingFeature::Create(RenderDevice& device)
	{
		drawPipeline_.Create(device);
	}
}
