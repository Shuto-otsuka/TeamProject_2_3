#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/ColorGrading/ColorGradingShader.h>
#include <GraphicsEngine/PostProcess/PostProcessFeature.h>

namespace SeedCore
{
	class ColorGradingFeature :public RenderFeature<ColorGradingFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Color, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::None, &PostProcessFeature::Color),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(ColorGradingConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<ColorGradingResolveDescription> resolvePipeline_;

		ColorGradingConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
