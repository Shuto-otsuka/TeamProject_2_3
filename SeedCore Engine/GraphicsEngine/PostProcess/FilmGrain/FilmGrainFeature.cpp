#include <GraphicsEngine/PostProcess/FilmGrain/FilmGrainFeature.h>
#include <GraphicsEngine/PostProcess/FilmGrain/FilmGrainStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(FilmGrainFeature, ResolveStage);

	void FilmGrainFeature::Create(RenderDevice& device)
	{
		resolvePipeline_.Create(device);
	}

	void FilmGrainFeature::Gather(const RenderScene& scene, RenderFrame& frame)
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
				enabled_ = postProcess.filmGrain_.enabled_;
				constants_.colored_ = postProcess.filmGrain_.colored_ ? 1 : 0;
				constants_.intensity_ = postProcess.filmGrain_.intensity_;
				constants_.size_ = postProcess.filmGrain_.size_;
				constants_.luminanceResponse_ = postProcess.filmGrain_.luminanceResponse_;
			});
	}

	void FilmGrainFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (enabled_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const FilmGrainConstantBuffer>(&constants_, 1));
		}
	}
}
