#include <GraphicsEngine/PostProcess/ColorGrading/ColorGradingFeature.h>
#include <GraphicsEngine/PostProcess/ColorGrading/ColorGradingStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(ColorGradingFeature, ResolveStage);

	void ColorGradingFeature::Create(RenderDevice& device)
	{
		resolvePipeline_.Create(device);
	}

	void ColorGradingFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		enabled_ = false;
		Bool found = false;
		Query<Read<Active>, Read<PostProcess>> query(*scene.world_);
		query.ForEach([&](const Active& active, const PostProcess& postProcess)
			{
				if (found || !active.active_)
				{
					return;
				}
				found = true;
				enabled_ = postProcess.colorGrading_.enabled_;
				constants_.shadowsMax_ = postProcess.colorGrading_.shadowsMax_;
				constants_.highlightsMin_ = postProcess.colorGrading_.highlightsMin_;
				StaticArray<const ColorGradingRangeSettings*, 4> ranges = { &postProcess.colorGrading_.global_, &postProcess.colorGrading_.shadows_, &postProcess.colorGrading_.midtones_, &postProcess.colorGrading_.highlights_ };
				for (Size rangeIndex = 0;rangeIndex < ranges.size();++rangeIndex)
				{
					const ColorGradingRangeSettings& range = *ranges[rangeIndex];
					constants_.ranges_[rangeIndex * 8 + 0] = range.temperature_;
					constants_.ranges_[rangeIndex * 8 + 1] = range.saturation_;
					constants_.ranges_[rangeIndex * 8 + 2] = range.contrast_;
					constants_.ranges_[rangeIndex * 8 + 3] = range.gamma_;
					constants_.ranges_[rangeIndex * 8 + 4] = range.gain_;
					constants_.ranges_[rangeIndex * 8 + 5] = range.offset_;
				}
			});
	}

	void ColorGradingFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (enabled_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const ColorGradingConstantBuffer>(&constants_, 1));
		}
	}
}
