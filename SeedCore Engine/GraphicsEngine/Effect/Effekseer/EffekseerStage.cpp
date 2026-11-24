#include <GraphicsEngine/Effect/Effekseer/EffekseerStage.h>
#include <GraphicsEngine/Effect/Effekseer/EffekseerManager.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>
#include <FoundationEngine/Resource/Gateway.h>

namespace SeedCore
{
	void EffekseerFeature::DrawStage::Execute(EffekseerFeature& feature, RenderPass& pass)
	{
		const RenderView* view = pass.View();
		if (!view)
		{
			return;
		}
		Gateway::GetEffekseerManager().Draw(pass.List(), view->scene_.view_, view->scene_.projection_);
	}
}