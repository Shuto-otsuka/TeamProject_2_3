#include <GraphicsEngine/Model/Opaque/GeometryFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(GeometryFeature, UpdateStage, DrawStage);

	void GeometryFeature::Create(RenderDevice& device)
	{
		updatePipeline_.Create(device);
		depthPipeline_.Create(device);
		visibilityPipeline_.Create(device);
		pyramidPipeline_.Create(device);
	}
}
