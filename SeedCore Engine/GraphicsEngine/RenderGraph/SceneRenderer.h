#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>

namespace SeedCore
{
	class SceneRenderer :public NonTransferable
	{
	public:
		explicit SceneRenderer(RenderDevice& device);

		~SceneRenderer();

		void Initialize(Uint64 capacity);

		void Render(const RenderScene& scene);

		NODISCARD const GpuProfiler& Profiler()const;

	private:
		RenderDevice& device_;

		RenderGraph graph_;

		RenderFrame frame_;

		DynamicArray<ResourcePtr<RenderFeatureInstance>> features_;
	};
}
