#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Culling/CullingShader.h>

namespace SeedCore
{
	class CullingFeature :public RenderFeature<CullingFeature>
	{
	private:
		SC_CONSTEX Uint32 capacity_ = 1048576;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(DrawLists, RenderResourceDesc::FixedBuffer(sizeof(Uint32) * 2, capacity_), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(DrawArguments, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 16), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Rejected, RenderResourceDesc::FixedBuffer(sizeof(Uint32) * 2, capacity_), RenderResourceScope::View, RenderResourceLifetime::Transient)
		);

	public:
		struct CameraEarlyStage;

		struct CameraLateStage;

	public:
		void Create(RenderDevice& device);

	private:
		ComputePipeline<CullingPrepareDescription> preparePipeline_;

		ComputePipeline<CullingEarlyDescription> earlyPipeline_;

		ComputePipeline<CullingLateDescription> latePipeline_;

		Bool mesh_ = false;
	};
}
