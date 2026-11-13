#include <GraphicsEngine/Illumination/DirectLight/DirectLightFeature.h>
#include <GraphicsEngine/Illumination/DirectLight/DirectLightStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(DirectLightFeature, ResolveStage);

	void DirectLightFeature::Create(RenderDevice& device)
	{
		ratioPipeline_.Create(device);
	}
}
