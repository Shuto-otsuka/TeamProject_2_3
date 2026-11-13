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

		SC_CONSTEX RenderFormat cubeFormat_ = RenderFormat::RGBA16Float;

		SC_CONSTEX RenderFormat brdfLookupTableFormat_ = RenderFormat::RG16Float;

		SC_CONSTEX Uint32 proceduralInterval_ = 120;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Cubemap, RenderResourceDesc::Cube(cubeFormat_, environmentSize_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(DiffuseIrradiance, RenderResourceDesc::Cube(cubeFormat_, irradianceSize_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(SpecularPrefiltered, RenderResourceDesc::Cube(cubeFormat_, prefilterSize_, prefilterMipLevels_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(BrdfLut, RenderResourceDesc::FixedTexture(brdfLookupTableFormat_, brdfLookupTableSize_, brdfLookupTableSize_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero)
		);

	public:
		struct BuildStage;

		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

	private:
		ComputePipeline<SkyEquirectDescription> equirectPipeline_;

		ComputePipeline<SkyProceduralDescription> proceduralPipeline_;

		ComputePipeline<SkyIrradianceDescription> irradiancePipeline_;

		ComputePipeline<SkyPrefilterDescription> prefilterPipeline_;

		ComputePipeline<SkyBrdfLutDescription> brdfLutPipeline_;

		ComputePipeline<SkyResolveDescription> resolvePipeline_;

		Uint32 sourceIndex_ = SC_INVALID;

		Uint32 proceduralHash_ = 0;

		Uint32 proceduralCounter_ = 0;

		Float time_ = 0.0f;

		Bool generated_ = false;

		Bool equirect_ = false;

		Bool proceduralGenerated_ = false;

		Bool procedural_ = false;

		Bool proceduralSky_ = false;

		Bool filtering_ = false;

		Bool brdfLutGenerated_ = false;

		Bool brdfLut_ = false;

		Bool resolving_ = false;
	};
}
