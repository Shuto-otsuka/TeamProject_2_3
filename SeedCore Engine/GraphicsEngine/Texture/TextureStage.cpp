#include <GraphicsEngine/Texture/TextureStage.h>
#include <GraphicsEngine/RenderGraph/RenderPass.h>

namespace SeedCore
{
	void TextureFeature::UpdateStage::Execute(TextureFeature& feature, RenderPass& pass)
	{
		Uint32 unitCount = static_cast<Uint32>(feature.units_.UnitUpdates().size());
		if (unitCount > 0)
		{
			feature.updatePipeline_.Dispatch(pass, unitCount, 1, 1, 0);
		}
		Uint32 quadCount = static_cast<Uint32>(feature.units_.QuadUpdates().size());
		if (quadCount > 0)
		{
			feature.updatePipeline_.Dispatch(pass, quadCount, 1, 1, 1);
		}
	}

	void TextureFeature::ExpandStage::Execute(TextureFeature& feature, RenderPass& pass)
	{
		Uint32 quadCount = feature.units_.Constants().quadCount_;
		if (quadCount > 0)
		{
			feature.expandPipeline_.Dispatch(pass, quadCount, 1, 1);
		}
	}

	void TextureFeature::SpriteStage::Execute(TextureFeature& feature, RenderPass& pass)
	{
		pass.ClearTarget(HUDFeature::Color);
		feature.spritePipeline_.Draw(pass, feature.units_.Constants().spriteCount_);
	}

	void TextureFeature::BillboardStage::Execute(TextureFeature& feature, RenderPass& pass)
	{
		feature.billboardPipeline_.Draw(pass, feature.units_.Constants().billboardCount_);
	}
}
