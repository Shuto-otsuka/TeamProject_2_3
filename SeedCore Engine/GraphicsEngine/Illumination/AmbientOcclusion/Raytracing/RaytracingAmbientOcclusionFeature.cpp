#include <GraphicsEngine/Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusionFeature.h>
#include <GraphicsEngine/Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusionStage.h>
#include <GraphicsEngine/Illumination/AmbientOcclusion/AmbientOcclusion.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(RaytracingAmbientOcclusionFeature, TraceStage, FilterStage);

	void RaytracingAmbientOcclusionFeature::Create(RenderDevice& device)
	{
		tracePipeline_.Create(device);
		temporalPipeline_.Create(device);
	}

	void RaytracingAmbientOcclusionFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		Bool traced = tracing_;
		tracing_ = false;
		Bool found = false;
		Query<Read<Active>, Read<AmbientOcclusion>> query(*scene.world_);
		query.ForEach([&](const Active& active, const AmbientOcclusion& ambientOcclusion)
			{
				if (found || !active.active_)
				{
					return;
				}
				found = true;
				tracing_ = ambientOcclusion.enable_ && ambientOcclusion.technique_ == AmbientOcclusionTechnique::Raytracing;
				constants_.rayLength_ = ambientOcclusion.radius_;
				constants_.normalBias_ = ambientOcclusion.raytracing_.normalBias_;
				constants_.power_ = ambientOcclusion.power_;
				constants_.rayCount_ = ambientOcclusion.raytracing_.rayCount_;
			});
		constants_.historyValid_ = traced && tracing_ ? 1u : 0u;
		++constants_.frame_;
	}

	void RaytracingAmbientOcclusionFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (tracing_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const RaytracingAmbientOcclusionConstantBuffer>(&constants_, 1));
		}
	}
}
