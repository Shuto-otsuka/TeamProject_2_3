#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionFeature.h>
#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionStage.h>
#include <GraphicsEngine/Illumination/Reflection/Reflection.h>
#include <GraphicsEngine/Quality/Upscale.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(RaytracingReflectionFeature, TraceStage, FilterStage);

	void RaytracingReflectionFeature::Create(RenderDevice& device)
	{
		tracePipeline_.Create(device);
		spatialPipeline_.Create(device);
		temporalPipeline_.Create(device);
		momentsPipeline_.Create(device);
		aTrousPipeline_.Create(device);
	}

	void RaytracingReflectionFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		Bool traced = tracing_;
		tracing_ = false;
		Bool found = false;
		Query<Read<Active>, Read<Reflection>> query(*scene.world_);
		query.ForEach([&](const Active& active, const Reflection& reflection)
			{
				if (found || !active.active_)
				{
					return;
				}
				found = true;
				tracing_ = reflection.enable_ && reflection.technique_ == ReflectionTechnique::Raytracing;
				constants_.rayTMax_ = reflection.raytracing_.rayTMax_;
				constants_.normalBias_ = reflection.raytracing_.normalBias_;
				constants_.strength_ = reflection.strength_;
				constants_.rayCount_ = reflection.raytracing_.rayCount_;
			});
		const GraphicsUpscale* upscale = scene.Find<GraphicsUpscale>();
		constants_.temporalReuse_ = upscale && upscale->method_ == UpscaleMethod::DlssRayReconstruction ? 0u : 1u;
		constants_.historyValid_ = traced && tracing_ ? 1u : 0u;
		++constants_.frame_;
	}

	void RaytracingReflectionFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (tracing_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const RaytracingReflectionConstantBuffer>(&constants_, 1));
		}
	}
}
