#include <GraphicsEngine/PostProcess/PostProcessFeature.h>
#include <GraphicsEngine/PostProcess/PostProcessStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(PostProcessFeature, ComposeStage);

	void PostProcessFeature::Create(RenderDevice& device)
	{
		composePipeline_.Create(device);
	}

	void PostProcessFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		constants_ = PostProcessComposeConstantBuffer{};
		Bool found = false;
		Query<Read<Active>, Read<PostProcess>> query(*scene.world_);
		query.ForEach([&](const Active& active, const PostProcess& postProcess)
			{
				if (found || !active.active_)
				{
					return;
				}
				found = true;
				constants_.exposureCompensation_ = postProcess.exposure_.compensation_;
				constants_.autoExposure_ = postProcess.exposure_.enabled_ ? 1 : 0;
				constants_.bloomIntensity_ = postProcess.bloom_.intensity_;
			});
	}

	void PostProcessFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		upload.Write(Constants, RenderViewKind::Game, std::span<const PostProcessComposeConstantBuffer>(&constants_, 1));
	}
}
