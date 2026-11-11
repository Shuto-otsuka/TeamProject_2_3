#include <GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareFeature.h>
#include <GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(AnamorphicFlareFeature, PrefilterStage, Blur1Stage, Blur2Stage, Blur3Stage, Blur4Stage, ComposeStage);

	void AnamorphicFlareFeature::Create(RenderDevice& device)
	{
		prefilterPipeline_.Create(device);
		blur1Pipeline_.Create(device);
		blur2Pipeline_.Create(device);
		blur3Pipeline_.Create(device);
		blur4Pipeline_.Create(device);
		composePipeline_.Create(device);
	}

	void AnamorphicFlareFeature::Gather(const RenderScene& scene, RenderFrame& frame)
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
				enabled_ = postProcess.anamorphicFlare_.enabled_;
				constants_.threshold_ = postProcess.anamorphicFlare_.threshold_;
				constants_.intensity_ = postProcess.anamorphicFlare_.intensity_;
				constants_.streakLength_ = postProcess.anamorphicFlare_.streakLength_;
				constants_.attenuation_ = postProcess.anamorphicFlare_.attenuation_;
				constants_.tint_ = { postProcess.anamorphicFlare_.tint_.r, postProcess.anamorphicFlare_.tint_.g, postProcess.anamorphicFlare_.tint_.b, postProcess.anamorphicFlare_.tint_.a };
			});
	}

	void AnamorphicFlareFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (enabled_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const AnamorphicFlareConstantBuffer>(&constants_, 1));
		}
	}
}
