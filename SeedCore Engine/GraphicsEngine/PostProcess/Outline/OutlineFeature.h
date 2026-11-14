#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/Outline/OutlineShader.h>
#include <GraphicsEngine/Model/ModelUnit.h>

namespace SeedCore
{
	class OutlineFeature :public RenderFeature<OutlineFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Styles, RenderResourceDesc::FixedBuffer(sizeof(OutlineStyleStructuredBuffer), ModelUnit::capacity_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_ACCUMULATION(Mask, RenderResourceDesc::ScaledTexture(RenderFormat::R32Uint), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Updates, RenderResourceDesc::FixedBuffer(sizeof(OutlineStyleUpdate), 1), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Source, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA32Uint), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Widest, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 1), RenderResourceScope::View, RenderResourceLifetime::Transient)
		);

	public:
		struct UpdateStage;
		struct ResolveStage;
		struct ComposeStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<OutlineStyleDescription> stylePipeline_;

		ComputePipeline<OutlineMaskDescription> maskPipeline_;

		ComputePipeline<OutlineResolveDescription> resolvePipeline_;

		ComputePipeline<OutlineCanvasDescription> canvasPipeline_;

		GraphicsPipeline<OutlineComposeDescription> composePipeline_;

		DynamicArray<OutlineStyleStructuredBuffer> styles_;

		DynamicArray<OutlineStyleStructuredBuffer> pending_;

		DynamicArray<Uint32> actors_;

		DynamicArray<Uint32> current_;

		DynamicArray<OutlineStyleUpdate> updates_;

		Bool updating_ = false;

		Bool outlining_ = false;
	};
}
