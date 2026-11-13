#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/Vignette/VignetteShader.h>
#include <GraphicsEngine/PostProcess/ChromaticAberration/ChromaticAberrationFeature.h>

namespace SeedCore
{
	class VignetteFeature :public RenderFeature<VignetteFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Color, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::None, &ChromaticAberrationFeature::Color),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(VignetteConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<VignetteResolveDescription> resolvePipeline_;

		VignetteConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
