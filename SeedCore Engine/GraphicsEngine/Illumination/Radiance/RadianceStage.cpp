#include <GraphicsEngine/Illumination/Radiance/RadianceStage.h>

namespace SeedCore
{
	void RadianceFeature::ComposeStage::Execute(RadianceFeature& feature, RenderPass& pass)
	{
		feature.composePipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}
}
