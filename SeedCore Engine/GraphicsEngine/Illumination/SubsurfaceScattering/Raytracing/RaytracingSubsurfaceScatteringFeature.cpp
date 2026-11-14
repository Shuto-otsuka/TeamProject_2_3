#include <GraphicsEngine/Illumination/SubsurfaceScattering/Raytracing/RaytracingSubsurfaceScatteringFeature.h>
#include <GraphicsEngine/Illumination/SubsurfaceScattering/Raytracing/RaytracingSubsurfaceScatteringStage.h>
#include <GraphicsEngine/Illumination/SubsurfaceScattering/SubsurfaceScattering.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(RaytracingSubsurfaceScatteringFeature, TraceStage);

	void RaytracingSubsurfaceScatteringFeature::Create(RenderDevice& device)
	{
		tracePipeline_.Create(device);
	}

	void RaytracingSubsurfaceScatteringFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		tracing_ = false;
		Bool found = false;
		Query<Read<Active>, Read<SubsurfaceScattering>> query(*scene.world_);
		query.ForEach([&](const Active& active, const SubsurfaceScattering& subsurfaceScattering)
			{
				if (found || !active.active_)
				{
					return;
				}
				found = true;
				tracing_ = subsurfaceScattering.enable_ && subsurfaceScattering.technique_ == SubsurfaceScatteringTechnique::Raytracing;
				constants_.scatteringDistance_ = subsurfaceScattering.scatteringDistance_;
				constants_.thicknessBias_ = subsurfaceScattering.raytracing_.thicknessBias_;
				constants_.rayTMax_ = subsurfaceScattering.raytracing_.rayTMax_;
				constants_.strength_ = subsurfaceScattering.strength_;
				constants_.subsurfaceColor_ = { subsurfaceScattering.subsurfaceColor_.r, subsurfaceScattering.subsurfaceColor_.g, subsurfaceScattering.subsurfaceColor_.b };
			});
	}

	void RaytracingSubsurfaceScatteringFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (tracing_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const RaytracingSubsurfaceScatteringConstantBuffer>(&constants_, 1));
		}
	}
}
