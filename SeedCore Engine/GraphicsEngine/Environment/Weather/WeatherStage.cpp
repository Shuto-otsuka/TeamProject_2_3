#include <GraphicsEngine/Environment/Weather/WeatherStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void WeatherFeature::UpdateStage::Execute(WeatherFeature& feature, RenderPass& pass)
	{
		Uint32 view = static_cast<Uint32>(pass.View()->kind_);
		feature.updatePipeline_.Dispatch(pass, rainCapacity_ + snowCapacity_, 1, 1, feature.spawned_[view] ? 0 : 1);
		feature.spawned_[view] = true;
	}

	void WeatherFeature::DrawStage::Execute(WeatherFeature& feature, RenderPass& pass)
	{
		feature.drawPipeline_.Draw(pass, feature.emitter_.rainCount_ + feature.emitter_.snowCount_);
	}
}
