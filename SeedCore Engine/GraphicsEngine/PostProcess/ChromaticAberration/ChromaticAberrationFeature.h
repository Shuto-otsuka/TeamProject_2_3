#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/ChromaticAberration/ChromaticAberrationShader.h>
#include <GraphicsEngine/PostProcess/LensDistortion/LensDistortionFeature.h>

namespace SeedCore
{
	class ChromaticAberrationFeature :public RenderFeature<ChromaticAberrationFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Color, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::None, &LensDistortionFeature::Color),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(ChromaticAberrationConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<ChromaticAberrationResolveDescription> resolvePipeline_;

		ChromaticAberrationConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
