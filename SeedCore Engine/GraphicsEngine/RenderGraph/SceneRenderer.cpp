#include <GraphicsEngine/RenderGraph/SceneRenderer.h>

namespace SeedCore
{
	SceneRenderer::SceneRenderer(RenderDevice& device) :device_(device), graph_(device)
	{
		/// No Code
	}

	SceneRenderer::~SceneRenderer() = default;

	void SceneRenderer::Initialize(Uint64 capacity)
	{
		graph_.Initialize(capacity);

		DynamicArray<RenderFeatureEntry> entries(RenderFeatureRegistry::Entries().begin(), RenderFeatureRegistry::Entries().end());
		std::ranges::sort(entries, [](const RenderFeatureEntry& a, const RenderFeatureEntry& b) { return a.featureName_ < b.featureName_; });
		for (const RenderFeatureEntry& entry : entries)
		{
			ResourcePtr<RenderFeatureInstance>& feature = features_.emplace_back(entry.create_());
			feature->Initialize(device_, graph_);
		}
	}

	void SceneRenderer::Render(const RenderScene& scene)
	{
		frame_ = RenderFrame{};
		for (ResourcePtr<RenderFeatureInstance>& feature : features_)
		{
			feature->Gather(scene, frame_);
		}
		for (ResourcePtr<RenderFeatureInstance>& feature : features_)
		{
			feature->Prepare(std::as_const(frame_), graph_.Upload());
		}
		graph_.Sort();
		graph_.Compile(scene);
		graph_.Execute(scene);
	}

	const GpuProfiler& SceneRenderer::Profiler()const
	{
		return graph_.Profiler();
	}
}
