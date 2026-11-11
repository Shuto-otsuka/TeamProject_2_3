#include <GraphicsEngine/PostProcess/Vignette/VignetteFeature.h>
#include <GraphicsEngine/PostProcess/Vignette/VignetteStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(VignetteFeature, ResolveStage);

	void VignetteFeature::Create(RenderDevice& device)
	{
		resolvePipeline_.Create(device);
	}

	void VignetteFeature::Gather(const RenderScene& scene, RenderFrame& frame)
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
				enabled_ = postProcess.vignette_.enabled_;
				constants_.intensity_ = postProcess.vignette_.intensity_;
				constants_.exponent_ = postProcess.vignette_.exponent_;
				constants_.color_ = { postProcess.vignette_.color_.r, postProcess.vignette_.color_.g, postProcess.vignette_.color_.b, postProcess.vignette_.color_.a };
			});
	}

	void VignetteFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (enabled_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const VignetteConstantBuffer>(&constants_, 1));
		}
	}
}
