#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(GeometryFeature, DepthEarlyStage, EarlyDepthResolveStage, DepthLateStage, DepthResolveStage, VisibilityStage);

	void GeometryFeature::Create(RenderDevice& device)
	{
		depthPipeline_.Create(device);
		visibilityPipeline_.Create(device);
		resolvePipeline_.Create(device);
	}
}
