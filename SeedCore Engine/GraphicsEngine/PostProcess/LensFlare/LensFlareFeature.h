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
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display, 4), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Bright, RenderResourceDesc::ScaledTextureArray(DXGI_FORMAT_R16G16B16A16_FLOAT, 1, RenderResourceScale::Display, 4), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Ping, RenderResourceDesc::ScaledTextureArray(DXGI_FORMAT_R16G16B16A16_FLOAT, axisCapacity_, RenderResourceScale::Display, 4), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Pong, RenderResourceDesc::ScaledTextureArray(DXGI_FORMAT_R16G16B16A16_FLOAT, axisCapacity_, RenderResourceScale::Display, 4), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(LensFlareConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct DownsampleStage;

		struct BlurPass1Stage;

		struct BlurPass2Stage;

		struct BlurPass3Stage;

		struct BlurPass4Stage;

		struct ComposeStage;

		struct GhostStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<LensFlareDownsampleDescription> downsamplePipeline_;

		ComputePipeline<LensFlareBlurPass1Description> blurPass1Pipeline_;

		ComputePipeline<LensFlareBlurPass2Description> blurPass2Pipeline_;

		ComputePipeline<LensFlareBlurPass3Description> blurPass3Pipeline_;

		ComputePipeline<LensFlareBlurPass4Description> blurPass4Pipeline_;

		ComputePipeline<LensFlareComposeDescription> composePipeline_;

		ComputePipeline<LensFlareGhostDescription> ghostPipeline_;

		LensFlareConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
