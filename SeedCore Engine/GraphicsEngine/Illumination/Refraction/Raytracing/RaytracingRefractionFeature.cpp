#include <GraphicsEngine/Illumination/Refraction/Raytracing/RaytracingRefractionFeature.h>
#include <GraphicsEngine/Illumination/Refraction/Raytracing/RaytracingRefractionStage.h>
#include <GraphicsEngine/Illumination/Refraction/Refraction.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(RaytracingRefractionFeature, TraceStage);

	void RaytracingRefractionFeature::Create(RenderDevice& device)
	{
		tracePipeline_.Create(device);
	}

	void RaytracingRefractionFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		tracing_ = false;
		Bool found = false;
		Query<Read<Active>, Read<Refraction>> query(*scene.world_);
		query.ForEach([&](const Active& active, const Refraction& refraction)
			{
				if (found || !active.active_)
				{
					return;
				}
				found = true;
				tracing_ = refraction.enable_ && refraction.technique_ == RefractionTechnique::Raytracing;
				constants_.rayTMax_ = refraction.raytracing_.rayTMax_;
				constants_.normalBias_ = refraction.raytracing_.normalBias_;
				constants_.strength_ = refraction.strength_;
			});
	}

	void RaytracingRefractionFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (tracing_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const RaytracingRefractionConstantBuffer>(&constants_, 1));
		}
	}
}
