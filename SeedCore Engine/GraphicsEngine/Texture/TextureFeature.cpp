#include <GraphicsEngine/Texture/TextureFeature.h>
#include <GraphicsEngine/Texture/TextureStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(TextureFeature, UpdateStage, ExpandStage, SpriteStage, BillboardStage);

	void TextureFeature::Create(RenderDevice& device)
	{
		updatePipeline_.Create(device);
		expandPipeline_.Create(device);
		spritePipeline_.Create(device);
		billboardPipeline_.Create(device);
	}

	void TextureFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		units_.Gather(scene);
		updating_ = !units_.UnitUpdates().empty() || !units_.QuadUpdates().empty();
		spriting_ = units_.Constants().spriteCount_ > 0;
		billboarding_ = units_.Constants().billboardCount_ > 0;
	}

	void TextureFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (!units_.UnitUpdates().empty())
		{
			upload.Write(UnitUpdates, RenderViewKind::Game, units_.UnitUpdates());
		}
		if (!units_.QuadUpdates().empty())
		{
			upload.Write(QuadUpdates, RenderViewKind::Game, units_.QuadUpdates());
		}
		upload.Write(Constants, RenderViewKind::Game, std::span<const TextureConstantBuffer>(&units_.Constants(), 1));
	}
}
