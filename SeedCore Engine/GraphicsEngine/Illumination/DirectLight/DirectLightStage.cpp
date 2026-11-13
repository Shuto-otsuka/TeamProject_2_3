#include <GraphicsEngine/Illumination/DirectLight/DirectLightStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void DirectLightFeature::ResolveStage::Execute(DirectLightFeature& feature, RenderPass& pass)
	{
		feature.ratioPipeline_.Dispatch(pass, pass.Width(), pass.Height(), 1);
	}
}
