#include <GraphicsEngine/PostProcess/LensFlare/LensFlareFeature.h>
#include <GraphicsEngine/PostProcess/LensFlare/LensFlareStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(LensFlareFeature, DownsampleStage, BlurPass1Stage, BlurPass2Stage, BlurPass3Stage, BlurPass4Stage, ComposeStage, GhostStage);

	void LensFlareFeature::Create(RenderDevice& device)
	{
		downsamplePipeline_.Create(device);
		blurPass1Pipeline_.Create(device);
		blurPass2Pipeline_.Create(device);
		blurPass3Pipeline_.Create(device);
		blurPass4Pipeline_.Create(device);
		composePipeline_.Create(device);
		ghostPipeline_.Create(device);
	}

	void LensFlareFeature::Gather(const RenderScene& scene, RenderFrame& frame)
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
				enabled_ = postProcess.lensFlare_.enabled_;
				constants_.threshold_ = postProcess.lensFlare_.threshold_;
				constants_.intensity_ = postProcess.lensFlare_.intensity_;
				constants_.streakLength_ = postProcess.lensFlare_.streakLength_;
				constants_.streakAttenuation_ = postProcess.lensFlare_.streakAttenuation_;
				constants_.chromaticAberration_ = postProcess.lensFlare_.chromaticAberration_;
				constants_.angleOffset_ = postProcess.lensFlare_.angleOffset_;
				constants_.ghostCount_ = postProcess.lensFlare_.ghostCount_;
				constants_.ghostDispersal_ = postProcess.lensFlare_.ghostDispersal_;
				constants_.ghostIntensity_ = postProcess.lensFlare_.ghostIntensity_;
				constants_.haloWidth_ = postProcess.lensFlare_.haloWidth_;
				constants_.spikeVariation_ = postProcess.lensFlare_.spikeVariation_;

				Uint32 bladeCount = Clamp(postProcess.bokeh_.bladeCount_, 3u, 8u);
				Uint32 axisCount = (bladeCount % 2 == 0) ? bladeCount / 2 : bladeCount;
				constants_.axisCount_ = Min(axisCount, axisCapacity_);
			});
	}

	void LensFlareFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (enabled_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const LensFlareConstantBuffer>(&constants_, 1));
		}
	}
}
