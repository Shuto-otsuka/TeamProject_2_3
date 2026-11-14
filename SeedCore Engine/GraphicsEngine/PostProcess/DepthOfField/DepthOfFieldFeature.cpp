#include <GraphicsEngine/PostProcess/DepthOfField/DepthOfFieldFeature.h>
#include <GraphicsEngine/PostProcess/DepthOfField/DepthOfFieldStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(DepthOfFieldFeature, FilterStage);

	void DepthOfFieldFeature::Create(RenderDevice& device)
	{
		blurPipeline_.Create(device);
	}

	void DepthOfFieldFeature::Gather(const RenderScene& scene, RenderFrame& frame)
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
				enabled_ = postProcess.depthOfField_.enabled_;
				constants_.focusDistance_ = postProcess.depthOfField_.focusDistance_;
				constants_.focusRange_ = postProcess.depthOfField_.focusRange_;
				constants_.maxBlurRadius_ = postProcess.depthOfField_.maxBlurRadius_;
			});
	}

	void DepthOfFieldFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (enabled_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const DepthOfFieldConstantBuffer>(&constants_, 1));
		}
	}
}
