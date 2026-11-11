#include <GraphicsEngine/PostProcess/LensDistortion/LensDistortionFeature.h>
#include <GraphicsEngine/PostProcess/LensDistortion/LensDistortionStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(LensDistortionFeature, ResolveStage);

	void LensDistortionFeature::Create(RenderDevice& device)
	{
		resolvePipeline_.Create(device);
	}

	void LensDistortionFeature::Gather(const RenderScene& scene, RenderFrame& frame)
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
				enabled_ = postProcess.lensDistortion_.enabled_;
				constants_.k1_ = postProcess.lensDistortion_.k1_;
				constants_.k2_ = postProcess.lensDistortion_.k2_;
				constants_.k3_ = postProcess.lensDistortion_.k3_;
				constants_.scale_ = postProcess.lensDistortion_.scale_;
			});
	}

	void LensDistortionFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (enabled_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const LensDistortionConstantBuffer>(&constants_, 1));
		}
	}
}
