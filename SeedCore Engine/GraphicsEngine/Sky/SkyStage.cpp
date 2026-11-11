#include <GraphicsEngine/Sky/SkyStage.h>

namespace SeedCore
{
	void SkyFeature::GenerateEquirectStage::Execute(SkyFeature& feature, RenderPass& pass)
	{
		feature.generateEquirectPipeline_.Dispatch(pass, environmentSize_, environmentSize_, 6, feature.sourceIndex_);
	}

	void SkyFeature::DiffuseIrradianceStage::Execute(SkyFeature& feature, RenderPass& pass)
	{
		feature.diffuseIrradiancePipeline_.Dispatch(pass, irradianceSize_, irradianceSize_, 6);
	}

	void SkyFeature::SpecularPrefilterStage::Execute(SkyFeature& feature, RenderPass& pass)
	{
		for (Uint32 mip = 0;mip < prefilterMipLevels_;++mip)
		{
			Uint32 size = Max(prefilterSize_ >> mip, 1u);
			feature.specularPrefilterPipeline_.Dispatch(pass, size, size, 6, mip | (prefilterMipLevels_ << 16));
		}
	}

	void SkyFeature::BrdfLutStage::Execute(SkyFeature& feature, RenderPass& pass)
	{
		feature.brdfLutPipeline_.Dispatch(pass, brdfLookupTableSize_, brdfLookupTableSize_);
	}

	void SkyFeature::ResolveStage::Execute(SkyFeature& feature, RenderPass& pass)
	{
		feature.resolvePipeline_.Dispatch(pass, pass.Width(), pass.Height());
	}
}
