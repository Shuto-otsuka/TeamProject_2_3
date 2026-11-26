#pragma once
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <GraphicsEngine/Graphics/GraphicsDisplay.h>
#include <GraphicsEngine/RenderGraph/RenderGraph.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>
#include <GraphicsEngine/Quality/Upscale.h>

namespace SeedCore
{
	struct CelestialResult;
	struct ShapeList;
	struct GraphicsSettings;

	class CameraSystem;
	class CanvasCamera;
	class EditorCamera;
	class GameTimer;
	class GpuProfiler;
	class GraphicsModule;
	class RenderDevice;
	class SceneRenderer;
	class World;
	class WorldTimer;

	class GraphicsScene
	{
	public:
		GraphicsScene(RenderDevice& device, GraphicsModule& module);

		~GraphicsScene();

		void Initialize();

		void Finalize();

	public:
		void Configure(const GraphicsSettings& settings);

	public:
		void Begin();

		void AddEditorView(WorldTimer& timer, const EditorCamera& editorCamera, World& world, RenderViewMode viewMode, RenderDebugSource debugSource, Uint64 showFlags);

		void AddGameView(GameTimer& timer, CameraSystem& cameraSystem, World& world, RenderViewOutput output, const Vector4& outputRect);

		void AddCanvasView(WorldTimer& timer, const CanvasCamera& canvasCamera, World& world);

		void Render(ID3D12Resource* screen, const CelestialResult& celestial, const ShapeList& shapes);

	public:
		NODISCARD Float AspectRatio()const;

		NODISCARD const GpuProfiler& Profiler()const;

		NODISCARD const GraphicsDisplay& Display()const;

	private:
		RenderDevice& device_;

		GraphicsModule& module_;

		Uint32 nativeWidth_ = static_cast<Uint32>(ScResolution::SC_HD.Width);

		Uint32 nativeHeight_ = static_cast<Uint32>(ScResolution::SC_HD.Height);

		Uint32 outputWidth_ = static_cast<Uint32>(ScResolution::SC_HD.Width);

		Uint32 outputHeight_ = static_cast<Uint32>(ScResolution::SC_HD.Height);

		GraphicsUpscale upscale_;

		Bool rendered_ = false;

		ResourcePtr<SceneRenderer> renderer_;

		GraphicsDisplay display_;

		StaticArray<RenderGraphAllocation, RenderViewCount> outputs_;

		Uint32 screenView_ = SC_INVALID;

		StaticArray<RenderView, RenderViewCount> views_;

		Uint32 viewCount_ = 0;

		World* world_ = nullptr;
	};
}
