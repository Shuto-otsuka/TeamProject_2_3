#include <GraphicsEngine/Screen/Splash/SplashStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void SplashFeature::DrawStage::Execute(SplashFeature& feature, RenderPass& pass)
	{
		feature.drawPipeline_.Draw(pass);
	}
}
