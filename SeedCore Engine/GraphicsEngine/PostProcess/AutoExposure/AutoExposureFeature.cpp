#include <GraphicsEngine/PostProcess/AutoExposure/AutoExposureFeature.h>
#include <GraphicsEngine/PostProcess/AutoExposure/AutoExposureStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(AutoExposureFeature, HistogramStage, AverageStage);

	void AutoExposureFeature::Create(RenderDevice& device)
	{
		histogramPipeline_.Create(device);
		averagePipeline_.Create(device);
	}

	void AutoExposureFeature::Gather(const RenderScene& scene, RenderFrame& frame)
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
				enabled_ = postProcess.exposure_.enabled_;
				constants_.minLogLuminance_ = postProcess.exposure_.minLogLuminance_;
				constants_.maxLogLuminance_ = postProcess.exposure_.maxLogLuminance_;
				constants_.keyValue_ = postProcess.exposure_.keyValue_;
				constants_.adaptSpeedToBright_ = postProcess.exposure_.adaptSpeedToBright_;
				constants_.adaptSpeedToDark_ = postProcess.exposure_.adaptSpeedToDark_;
			});
	}

	void AutoExposureFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (enabled_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const AutoExposureConstantBuffer>(&constants_, 1));
		}
	}
}
