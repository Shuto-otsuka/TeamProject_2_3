#include <GraphicsEngine/Sky/SkyStage.h>

namespace SeedCore
{
	void SkyFeature::BuildStage::EquirectStep::Execute(SkyFeature& feature, RenderPass& pass)
	{
		feature.equirectPipeline_.Dispatch(pass, environmentSize_, environmentSize_, 6, feature.sourceIndex_);
	}

	void SkyFeature::BuildStage::ProceduralStep::Execute(SkyFeature& feature, RenderPass& pass)
	{
		feature.proceduralPipeline_.Dispatch(pass, environmentSize_, environmentSize_, 6, std::bit_cast<Uint32>(feature.time_));
	}

	void SkyFeature::BuildStage::IrradianceStep::Execute(SkyFeature& feature, RenderPass& pass)
	{
		feature.irradiancePipeline_.Dispatch(pass, irradianceSize_, irradianceSize_, 6);
	}

	void SkyFeature::BuildStage::PrefilterStep::Execute(SkyFeature& feature, RenderPass& pass)
	{
		for (Uint32 mip = 0;mip < prefilterMipLevels_;++mip)
		{
			Uint32 size = Max(prefilterSize_ >> mip, 1u);
			feature.prefilterPipeline_.Dispatch(pass, size, size, 6, mip | (prefilterMipLevels_ << 16));
		}
	}

	void SkyFeature::BuildStage::BrdfLutStep::Execute(SkyFeature& feature, RenderPass& pass)
	{
		feature.brdfLutPipeline_.Dispatch(pass, brdfLookupTableSize_, brdfLookupTableSize_);
	}

	void SkyFeature::ResolveStage::Execute(SkyFeature& feature, RenderPass& pass)
	{
		Uint32 source = feature.proceduralSky_ ? 1 : (feature.resolving_ ? 0 : 2);
		feature.resolvePipeline_.Dispatch(pass, pass.Width(), pass.Height(), 1, source);
	}
}
