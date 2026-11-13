#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomShader.h>

namespace SeedCore
{
	class KawaseBloomFeature :public RenderFeature<KawaseBloomFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display, 2), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Level1, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display, 4), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Level2, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display, 8), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Level3, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display, 16), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Level4, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display, 32), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Level5, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display, 64), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(KawaseBloomConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct FilterStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<KawaseBloomPrefilterDescription> prefilterPipeline_;

		ComputePipeline<KawaseBloomDownsampleDescription> downsamplePipeline_;

		ComputePipeline<KawaseBloomUpsampleDescription> upsamplePipeline_;

		KawaseBloomConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
