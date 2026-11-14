#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLightShader.h>

namespace SeedCore
{
	class RaytracingVolumetricLightFeature :public RenderFeature<RaytracingVolumetricLightFeature>
	{
	public:
		SC_CONSTEX Uint32 froxelDivisor_ = 12;

		SC_CONSTEX Uint32 froxelDepth_ = 128;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Fog, RenderResourceDesc::Volume(RenderFormat::RGBA16Float, RenderResourceScale::Render, froxelDivisor_, froxelDepth_), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Density, RenderResourceDesc::Volume(RenderFormat::RGBA16Float, RenderResourceScale::Render, froxelDivisor_, froxelDepth_), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Scattering, RenderResourceDesc::Volume(RenderFormat::RGBA16Float, RenderResourceScale::Render, froxelDivisor_, froxelDepth_), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(RaytracingVolumetricLightConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct BuildStage;

		struct TraceStage;

		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<RaytracingVolumetricLightInjectDescription> injectPipeline_;

		ComputePipeline<RaytracingVolumetricLightScatterDescription> scatterPipeline_;

		ComputePipeline<RaytracingVolumetricLightIntegrateDescription> integratePipeline_;

		RaytracingVolumetricLightConstantBuffer constants_;

		Bool tracing_ = false;
	};
}
