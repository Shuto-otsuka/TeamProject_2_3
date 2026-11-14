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
			SC_RENDER_CONTRACT(OpaqueMeshlets, RenderResourceDesc::FixedBuffer(sizeof(Uint32) * 2, capacity_), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(MeshletArguments, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 16), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(TransparentMeshlets, RenderResourceDesc::FixedBuffer(sizeof(Uint32) * 2, capacity_), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(TransparentArguments, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 8), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(OpaqueChunks, RenderResourceDesc::FixedBuffer(sizeof(Uint32), capacity_), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(OccludedChunks, RenderResourceDesc::FixedBuffer(sizeof(Uint32), capacity_), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(ChunkArguments, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 16), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(TransparentChunks, RenderResourceDesc::FixedBuffer(sizeof(Uint32), capacity_), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(OccludedMeshlets, RenderResourceDesc::FixedBuffer(sizeof(Uint32) * 2, capacity_), RenderResourceScope::View, RenderResourceLifetime::Transient)
		);

	public:
		struct CullingStage;

	public:
		void Create(RenderDevice& device);

	private:
		ComputePipeline<CullingPrepareDescription> preparePipeline_;

		ComputePipeline<CullingEarlyDescription> earlyPipeline_;

		ComputePipeline<CullingLateDescription> latePipeline_;

		ComputePipeline<CullingTransparentDescription> transparentPipeline_;

		Bool mesh_ = false;
	};
}
