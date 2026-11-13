#include <GraphicsEngine/Model/Transparent/TransparentFeature.h>
#include <GraphicsEngine/Model/Transparent/TransparentStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(TransparentFeature, UpdateStage, DrawStage, ResolveStage);

	void TransparentFeature::Create(RenderDevice& device)
	{
		clearPipeline_.Create(device);
		drawPipeline_.Create(device);
		resolvePipeline_.Create(device);
	}
}
