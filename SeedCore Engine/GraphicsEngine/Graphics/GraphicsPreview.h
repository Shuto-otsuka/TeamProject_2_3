#pragma once
#include <GraphicsEngine/Preview/PreviewRequest.h>
#include <GraphicsEngine/RenderGraph/RenderGraph.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>
#include <GraphicsEngine/Screen/Boot/BootRequest.h>

namespace SeedCore
{
	class GraphicsModule;
	class PreviewCamera;
	class RenderDevice;
	class SceneRenderer;

	class GraphicsPreview
	{
	public:
		GraphicsPreview(RenderDevice& device, GraphicsModule& module);

		~GraphicsPreview();

		void Finalize();

	public:
		void Add(PreviewSlot slot, const PreviewRequest& request, const PreviewCamera& camera, Float totalTime);

		void Add(const BootRequest& request, Uint32 width, Uint32 height);

		void Render();

	public:
		NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE Display(PreviewSlot slot)const;

	private:
		struct Slot
		{
			ResourcePtr<SceneRenderer> renderer_;

			RenderGraphAllocation output_;

			PreviewRequest request_;

			BootRequest boot_;

			RenderView view_;

			Bool requested_ = false;
		};

	private:
		RenderDevice& device_;

		GraphicsModule& module_;

		StaticArray<Slot, PreviewSlotCount> slots_;
	};
}
