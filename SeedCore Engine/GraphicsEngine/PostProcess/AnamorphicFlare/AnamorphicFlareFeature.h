#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/AnamorphicFlare/AnamorphicFlareShader.h>

namespace SeedCore
{
	class AnamorphicFlareFeature :public RenderFeature<AnamorphicFlareFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display, 4), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Ping, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display, 8, 4, 1), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Pong, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display, 8, 4, 1), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(AnamorphicFlareConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct PrefilterStage;

		struct Blur1Stage;

		struct Blur2Stage;

		struct Blur3Stage;

		struct Blur4Stage;

		struct ComposeStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<AnamorphicFlarePrefilterDescription> prefilterPipeline_;

		ComputePipeline<AnamorphicFlareBlur1Description> blur1Pipeline_;

		ComputePipeline<AnamorphicFlareBlur2Description> blur2Pipeline_;

		ComputePipeline<AnamorphicFlareBlur3Description> blur3Pipeline_;

		ComputePipeline<AnamorphicFlareBlur4Description> blur4Pipeline_;

		ComputePipeline<AnamorphicFlareComposeDescription> composePipeline_;

		AnamorphicFlareConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
