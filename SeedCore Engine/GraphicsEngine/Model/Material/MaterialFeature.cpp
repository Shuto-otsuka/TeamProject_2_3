#include <GraphicsEngine/Model/Material/MaterialFeature.h>
#include <GraphicsEngine/Model/Material/MaterialStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(MaterialFeature, ClassifyStage, PrefixSumStage, ScatterStage, ResolveStage);

	void MaterialFeature::Create(RenderDevice& device)
	{
		preparePipeline_.Create(device);
		classifyPipeline_.Create(device);
		prefixSumPipeline_.Create(device);
		scatterPipeline_.Create(device);
		resolvePipeline_.Create(device);
	}
}
