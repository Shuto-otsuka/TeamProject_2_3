#include <GraphicsEngine/PostProcess/Bokeh/BokehFeature.h>
#include <GraphicsEngine/PostProcess/Bokeh/BokehStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(BokehFeature, ResolveStage);

	void BokehFeature::Create(RenderDevice& device)
	{
		resolvePipeline_.Create(device);
	}

	void BokehFeature::Gather(const RenderScene& scene, RenderFrame& frame)
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
				enabled_ = postProcess.depthOfField_.enabled_ && postProcess.bokeh_.enabled_;
				constants_.focusDistance_ = postProcess.depthOfField_.focusDistance_;
				constants_.focusRange_ = postProcess.depthOfField_.focusRange_;
				constants_.maxBlurRadius_ = postProcess.depthOfField_.maxBlurRadius_;
				constants_.highlightThreshold_ = postProcess.bokeh_.highlightThreshold_;
				constants_.highlightIntensity_ = postProcess.bokeh_.highlightIntensity_;
				constants_.bladeCount_ = postProcess.bokeh_.bladeCount_;
			});
	}

	void BokehFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (enabled_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const BokehConstantBuffer>(&constants_, 1));
		}
	}
}
