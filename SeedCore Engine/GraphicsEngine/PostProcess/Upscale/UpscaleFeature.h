#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/Upscale/UpscaleShader.h>
#include <GraphicsEngine/Illumination/Radiance/RadianceFeature.h>
#include <GraphicsEngine/Quality/Upscale.h>

namespace SeedCore
{
	class UpscaleFeature :public RenderFeature<UpscaleFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Color, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::None, &RadianceFeature::Color),
			SC_RENDER_INTERNAL(History, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(NormalRoughness, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(SpecularAlbedo, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(DiffuseAlbedo, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT), RenderResourceScope::View, RenderResourceLifetime::Transient)
		);

	public:
		struct TaauResolveStage;

		struct PrepareGuidesStage;

		struct DlssRayReconstructionStage;

		struct DlssSuperResolutionStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

	private:
		ComputePipeline<UpscaleTaauResolveDescription> taauResolvePipeline_;

		ComputePipeline<UpscalePrepareGuidesDescription> prepareGuidesPipeline_;

		UpscaleMode mode_ = UpscaleMode::MaxQuality;

		Bool taau_ = false;

		Bool dlssRayReconstruction_ = false;

		Bool dlssSuperResolution_ = false;
	};
}
