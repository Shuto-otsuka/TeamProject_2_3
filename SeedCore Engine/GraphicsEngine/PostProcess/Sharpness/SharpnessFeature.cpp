#include <GraphicsEngine/PostProcess/Sharpness/SharpnessFeature.h>
#include <GraphicsEngine/PostProcess/Sharpness/SharpnessStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(SharpnessFeature, ResolveStage);

	void SharpnessFeature::Create(RenderDevice& device)
	{
		resolvePipeline_.Create(device);
	}

	void SharpnessFeature::Gather(const RenderScene& scene, RenderFrame& frame)
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
				enabled_ = postProcess.sharpness_.enabled_;
				constants_.amount_ = postProcess.sharpness_.amount_;
			});
	}

	void SharpnessFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (enabled_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const SharpnessConstantBuffer>(&constants_, 1));
		}
	}
}
