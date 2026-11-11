#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Sky/SkyShader.h>

namespace SeedCore
{
	class SkyFeature :public RenderFeature<SkyFeature>
	{
	private:
		SC_CONSTEX Uint32 environmentSize_ = 512;

		SC_CONSTEX Uint32 irradianceSize_ = 32;

		SC_CONSTEX Uint32 prefilterSize_ = 128;

		SC_CONSTEX Uint32 prefilterMipLevels_ = 5;

		SC_CONSTEX Uint32 brdfLookupTableSize_ = 512;

		SC_CONSTEX DXGI_FORMAT cubeFormat_ = DXGI_FORMAT_R16G16B16A16_FLOAT;

		SC_CONSTEX DXGI_FORMAT brdfLookupTableFormat_ = DXGI_FORMAT_R16G16_FLOAT;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Cubemap, RenderResourceDesc::Cube(cubeFormat_, environmentSize_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(DiffuseIrradiance, RenderResourceDesc::Cube(cubeFormat_, irradianceSize_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(SpecularPrefiltered, RenderResourceDesc::Cube(cubeFormat_, prefilterSize_, prefilterMipLevels_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(BrdfLut, RenderResourceDesc::FixedTexture(brdfLookupTableFormat_, brdfLookupTableSize_, brdfLookupTableSize_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero)
		);

	public:
		struct GenerateEquirectStage;

		struct DiffuseIrradianceStage;

		struct SpecularPrefilterStage;

		struct BrdfLutStage;

		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

	private:
		ComputePipeline<SkyGenerateEquirectDescription> generateEquirectPipeline_;

		ComputePipeline<SkyDiffuseIrradianceDescription> diffuseIrradiancePipeline_;

		ComputePipeline<SkySpecularPrefilterDescription> specularPrefilterPipeline_;

		ComputePipeline<SkyBrdfLutDescription> brdfLutPipeline_;

		ComputePipeline<SkyResolveDescription> resolvePipeline_;

		Uint32 sourceIndex_ = SC_INVALID;

		Bool generated_ = false;

		Bool generate_ = false;

		Bool brdfLutGenerated_ = false;

		Bool brdfLut_ = false;

		Bool resolving_ = false;
	};
}
