#include <GraphicsEngine/PostProcess/ToneMapping/ToneMappingFeature.h>
#include <GraphicsEngine/PostProcess/ToneMapping/ToneMappingStage.h>
#include <GraphicsEngine/PostProcess/PostProcess.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(ToneMappingFeature, ResolveStage);

	void ToneMappingFeature::Create(RenderDevice& device)
	{
		resolvePipeline_.Create(device);
	}

	void ToneMappingFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		constants_ = ToneMappingConstantBuffer{};
		Bool found = false;
		Query<Read<Active>, Read<PostProcess>> query(*scene.world_);
		query.ForEach([&](const Active& active, const PostProcess& postProcess)
			{
				if (found || !active.active_)
				{
					return;
				}
				found = true;
				constants_.enabled_ = postProcess.toneMapping_.enabled_ ? 1 : 0;
				constants_.mode_ = static_cast<Uint32>(postProcess.toneMapping_.mode_);
			});
	}

	void ToneMappingFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		upload.Write(Constants, RenderViewKind::Game, std::span<const ToneMappingConstantBuffer>(&constants_, 1));
	}
}
