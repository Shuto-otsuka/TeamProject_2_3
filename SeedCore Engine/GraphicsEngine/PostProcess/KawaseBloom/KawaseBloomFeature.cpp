#include <GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomFeature.h>
#include <GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(KawaseBloomFeature, FilterStage);

	void KawaseBloomFeature::Create(RenderDevice& device)
	{
		prefilterPipeline_.Create(device);
		downsamplePipeline_.Create(device);
		upsamplePipeline_.Create(device);
	}

	void KawaseBloomFeature::Gather(const RenderScene& scene, RenderFrame& frame)
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
				enabled_ = postProcess.bloom_.enabled_;
				constants_.threshold_ = postProcess.bloom_.threshold_;
				constants_.softKnee_ = postProcess.bloom_.softKnee_;
				constants_.filterRadius_ = postProcess.bloom_.filterRadius_;
			});
	}

	void KawaseBloomFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (enabled_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const KawaseBloomConstantBuffer>(&constants_, 1));
		}
	}
}
