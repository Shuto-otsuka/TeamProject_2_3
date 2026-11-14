#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/LensFlare/LensFlareShader.h>

namespace SeedCore
{
	class LensFlareFeature :public RenderFeature<LensFlareFeature>
	{
	private:
		SC_CONSTEX Uint32 axisCapacity_ = 7;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display, 4), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Bright, RenderResourceDesc::ScaledTextureArray(RenderFormat::RGBA16Float, 1, RenderResourceScale::Display, 4), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Ping, RenderResourceDesc::ScaledTextureArray(RenderFormat::RGBA16Float, axisCapacity_, RenderResourceScale::Display, 4), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Pong, RenderResourceDesc::ScaledTextureArray(RenderFormat::RGBA16Float, axisCapacity_, RenderResourceScale::Display, 4), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(LensFlareConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct FilterStage;

		struct ComposeStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<LensFlareDownsampleDescription> downsamplePipeline_;

		ComputePipeline<LensFlareBlurDescription> blurPipeline_;

		ComputePipeline<LensFlareGlareDescription> glarePipeline_;

		ComputePipeline<LensFlareGhostDescription> ghostPipeline_;

		LensFlareConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
