#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/LensDistortion/LensDistortionShader.h>
#include <GraphicsEngine/PostProcess/Bokeh/BokehFeature.h>

namespace SeedCore
{
	class LensDistortionFeature :public RenderFeature<LensDistortionFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Color, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::None, &BokehFeature::Color),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(LensDistortionConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<LensDistortionResolveDescription> resolvePipeline_;

		LensDistortionConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
