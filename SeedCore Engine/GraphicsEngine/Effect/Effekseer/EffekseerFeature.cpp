#include <GraphicsEngine/Effect/Effekseer/EffekseerFeature.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerStage.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerManager.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <FoundationEngine/Resource/Gateway.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(EffekseerFeature, DrawStage);

	void EffekseerFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		active_ = Gateway::GetEffekseerManager().GetManager()->GetTotalInstanceCount() > 0;
	}
}