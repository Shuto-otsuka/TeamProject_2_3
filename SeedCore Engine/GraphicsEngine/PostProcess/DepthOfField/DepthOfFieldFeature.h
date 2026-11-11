#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/DepthOfField/DepthOfFieldShader.h>
#include <GraphicsEngine/PostProcess/Upscale/UpscaleFeature.h>

namespace SeedCore
{
	class DepthOfFieldFeature :public RenderFeature<DepthOfFieldFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Color, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::None, &UpscaleFeature::Color),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(DepthOfFieldConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct BlurStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<DepthOfFieldBlurDescription> blurPipeline_;

		DepthOfFieldConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
