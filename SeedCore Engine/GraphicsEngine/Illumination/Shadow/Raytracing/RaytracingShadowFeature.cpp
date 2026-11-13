#include <GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadowFeature.h>
#include <GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadowStage.h>
#include <GraphicsEngine/Illumination/Shadow/Shadow.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(RaytracingShadowFeature, TraceStage, FilterStage);

	void RaytracingShadowFeature::Create(RenderDevice& device)
	{
		tracePipeline_.Create(device);
		temporalPipeline_.Create(device);
		momentsPipeline_.Create(device);
		aTrousPipeline_.Create(device);
	}

	void RaytracingShadowFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		Bool traced = tracing_;
		tracing_ = false;
		Bool found = false;
		Query<Read<Active>, Read<Shadow>> query(*scene.world_);
		query.ForEach([&](const Active& active, const Shadow& shadow)
			{
				if (found || !active.active_)
				{
					return;
				}
				found = true;
				tracing_ = shadow.enable_ && shadow.technique_ == ShadowTechnique::Raytracing;
				constants_.rayTMax_ = shadow.raytracing_.rayTMax_;
				constants_.normalBias_ = shadow.raytracing_.normalBias_;
				constants_.strength_ = shadow.raytracing_.shadowStrength_;
				constants_.sunAngularRadius_ = shadow.raytracing_.sunAngularRadius_;
				constants_.punctualRadius_ = shadow.raytracing_.punctualLightRadius_;
				constants_.rayCount_ = shadow.raytracing_.rayCount_;
			});
		constants_.historyValid_ = traced && tracing_ ? 1u : 0u;
		++constants_.frame_;
	}

	void RaytracingShadowFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (tracing_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const RaytracingShadowConstantBuffer>(&constants_, 1));
		}
	}
}
