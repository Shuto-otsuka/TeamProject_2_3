#include <GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationFeature.h>
#include <GraphicsEngine/Illumination/GlobalIllumination/Raytracing/RaytracingGlobalIlluminationStage.h>
#include <GraphicsEngine/Illumination/GlobalIllumination/GlobalIllumination.h>
#include <GraphicsEngine/Quality/Upscale.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(RaytracingGlobalIlluminationFeature, TraceStage, FilterStage);

	void RaytracingGlobalIlluminationFeature::Create(RenderDevice& device)
	{
		tracePipeline_.Create(device);
		spatialPipeline_.Create(device);
		temporalPipeline_.Create(device);
		aTrousPipeline_.Create(device);
	}

	void RaytracingGlobalIlluminationFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		Bool traced = tracing_;
		tracing_ = false;
		Bool found = false;
		Query<Read<Active>, Read<GlobalIllumination>> query(*scene.world_);
		query.ForEach([&](const Active& active, const GlobalIllumination& globalIllumination)
			{
				if (found || !active.active_)
				{
					return;
				}
				found = true;
				tracing_ = globalIllumination.enable_ && globalIllumination.technique_ == GlobalIlluminationTechnique::Raytracing;
				constants_.rayTMax_ = globalIllumination.raytracing_.rayTMax_;
				constants_.normalBias_ = globalIllumination.raytracing_.normalBias_;
				constants_.intensity_ = globalIllumination.intensity_;
				constants_.rayCount_ = globalIllumination.raytracing_.rayCount_;
				constants_.bounceCount_ = globalIllumination.raytracing_.bounceCount_;
			});
		const GraphicsUpscale* upscale = scene.Find<GraphicsUpscale>();
		constants_.temporalReuse_ = upscale && upscale->method_ == UpscaleMethod::DlssRayReconstruction ? 0u : 1u;
		constants_.historyValid_ = traced && tracing_ ? 1u : 0u;
		++constants_.frame_;
	}

	void RaytracingGlobalIlluminationFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (tracing_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const RaytracingGlobalIlluminationConstantBuffer>(&constants_, 1));
		}
	}
}
