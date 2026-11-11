#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Debug/DebugViewShader.h>

namespace SeedCore
{
	class DebugViewFeature :public RenderFeature<DebugViewFeature>
	{
	public:
		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

	private:
		ComputePipeline<DebugViewResolveDescription> resolvePipeline_;

		Bool resolving_[RenderViewCount] = { false, false, false };
	};
}
