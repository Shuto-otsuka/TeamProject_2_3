#include <GraphicsEngine/Illumination/Radiance/RadianceFeature.h>
#include <GraphicsEngine/Illumination/Radiance/RadianceStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(RadianceFeature, ComposeStage);

	void RadianceFeature::Create(RenderDevice& device)
	{
		composePipeline_.Create(device);
	}
}
