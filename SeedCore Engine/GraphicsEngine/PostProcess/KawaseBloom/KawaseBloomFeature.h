#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/KawaseBloom/KawaseBloomShader.h>

namespace SeedCore
{
	class KawaseBloomFeature :public RenderFeature<KawaseBloomFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display, 2), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Level1, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display, 4), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Level2, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display, 8), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Level3, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display, 16), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Level4, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display, 32), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Level5, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display, 64), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(KawaseBloomConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct PrefilterStage;

		struct Downsample1Stage;

		struct Downsample2Stage;

		struct Downsample3Stage;

		struct Downsample4Stage;

		struct Downsample5Stage;

		struct Upsample4Stage;

		struct Upsample3Stage;

		struct Upsample2Stage;

		struct Upsample1Stage;

		struct Upsample0Stage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<KawaseBloomPrefilterDescription> prefilterPipeline_;

		ComputePipeline<KawaseBloomDownsample1Description> downsample1Pipeline_;

		ComputePipeline<KawaseBloomDownsample2Description> downsample2Pipeline_;

		ComputePipeline<KawaseBloomDownsample3Description> downsample3Pipeline_;

		ComputePipeline<KawaseBloomDownsample4Description> downsample4Pipeline_;

		ComputePipeline<KawaseBloomDownsample5Description> downsample5Pipeline_;

		ComputePipeline<KawaseBloomUpsample4Description> upsample4Pipeline_;

		ComputePipeline<KawaseBloomUpsample3Description> upsample3Pipeline_;

		ComputePipeline<KawaseBloomUpsample2Description> upsample2Pipeline_;

		ComputePipeline<KawaseBloomUpsample1Description> upsample1Pipeline_;

		ComputePipeline<KawaseBloomUpsample0Description> upsample0Pipeline_;

		KawaseBloomConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
