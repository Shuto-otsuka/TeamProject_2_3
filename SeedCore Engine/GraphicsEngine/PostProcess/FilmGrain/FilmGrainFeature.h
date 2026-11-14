#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/FilmGrain/FilmGrainShader.h>

namespace SeedCore
{
	class FilmGrainFeature :public RenderFeature<FilmGrainFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(FilmGrainConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<FilmGrainResolveDescription> resolvePipeline_;

		FilmGrainConstantBuffer constants_;

		Bool enabled_ = false;
	};
}
