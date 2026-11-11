#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/ToneMapping/ToneMappingShader.h>
#include <GraphicsEngine/PostProcess/ColorGrading/ColorGradingFeature.h>

namespace SeedCore
{
	class ToneMappingFeature :public RenderFeature<ToneMappingFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Color, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(ToneMappingConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<ToneMappingResolveDescription> resolvePipeline_;

		ToneMappingConstantBuffer constants_;
	};
}
