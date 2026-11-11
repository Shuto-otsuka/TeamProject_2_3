#include <GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomFeature.h>
#include <GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(KawaseBloomFeature, PrefilterStage, Downsample1Stage, Downsample2Stage, Downsample3Stage, Downsample4Stage, Downsample5Stage, Upsample4Stage, Upsample3Stage, Upsample2Stage, Upsample1Stage, Upsample0Stage);

	void KawaseBloomFeature::Create(RenderDevice& device)
	{
		prefilterPipeline_.Create(device);
		downsample1Pipeline_.Create(device);
		downsample2Pipeline_.Create(device);
		downsample3Pipeline_.Create(device);
		downsample4Pipeline_.Create(device);
		downsample5Pipeline_.Create(device);
		upsample4Pipeline_.Create(device);
		upsample3Pipeline_.Create(device);
		upsample2Pipeline_.Create(device);
		upsample1Pipeline_.Create(device);
		upsample0Pipeline_.Create(device);
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
