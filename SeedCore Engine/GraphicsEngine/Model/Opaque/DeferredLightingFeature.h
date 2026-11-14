#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Model/Opaque/DeferredLightingShader.h>

namespace SeedCore
{
	class DeferredLightingFeature :public RenderFeature<DeferredLightingFeature>
	{
	public:
		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

	private:
		GraphicsPipeline<DeferredLightingResolveDescription> resolvePipeline_;
	};
}
