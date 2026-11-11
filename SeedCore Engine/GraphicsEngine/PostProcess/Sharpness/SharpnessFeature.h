#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/Sharpness/SharpnessShader.h>
#include <GraphicsEngine/PostProcess/ToneMapping/ToneMappingFeature.h>

namespace SeedCore
{
	class SharpnessFeature :public RenderFeature<SharpnessFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Color, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::None, &ToneMappingFeature::Color),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(SharpnessConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<SharpnessResolveDescription> resolvePipeline_;

		SharpnessConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
