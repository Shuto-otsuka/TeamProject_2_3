#include <GraphicsEngine/PostProcess/ChromaticAberration/ChromaticAberrationFeature.h>
#include <GraphicsEngine/PostProcess/ChromaticAberration/ChromaticAberrationStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(ChromaticAberrationFeature, ResolveStage);

	void ChromaticAberrationFeature::Create(RenderDevice& device)
	{
		resolvePipeline_.Create(device);
	}

	void ChromaticAberrationFeature::Gather(const RenderScene& scene, RenderFrame& frame)
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
				enabled_ = postProcess.chromaticAberration_.enabled_;
				constants_.intensity_ = postProcess.chromaticAberration_.intensity_;
				constants_.sampleCount_ = postProcess.chromaticAberration_.sampleCount_;
			});
	}

	void ChromaticAberrationFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (enabled_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const ChromaticAberrationConstantBuffer>(&constants_, 1));
		}
	}
}
