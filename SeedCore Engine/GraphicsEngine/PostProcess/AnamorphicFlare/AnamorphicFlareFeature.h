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
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display, 4), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Ping, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display, 8, 4, 1), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Pong, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display, 8, 4, 1), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(AnamorphicFlareConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct FilterStage;

		struct ComposeStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<AnamorphicFlarePrefilterDescription> prefilterPipeline_;

		ComputePipeline<AnamorphicFlareBlurDescription> blurPipeline_;

		ComputePipeline<AnamorphicFlareComposeDescription> composePipeline_;

		AnamorphicFlareConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
