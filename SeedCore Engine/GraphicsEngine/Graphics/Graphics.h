#pragma once
#include <GraphicsEngine/Quality/Upscale.h>

namespace SeedCore
{
	enum class RenderViewMode :Uint32;
	enum class RenderDebugSource :Uint32;
	enum class RenderViewOutput :Uint8;
	enum class PreviewSlot :Uint8;

	struct BootRequest;
	struct GameConfig;
	struct PreviewRequest;

	class CameraSystem;
	class CanvasCamera;
	class EditorCamera;
	class GameTimer;
	class GpuProfiler;
	class GraphicsDisplay;
	class GraphicsModule;
	class PreviewCamera;
	class RenderDevice;
	class SplashSystem;
	class Window;
	class World;
	class WorldTimer;

	struct GraphicsSettings
	{
		Uint32 outputWidth_ = 0;

		Uint32 outputHeight_ = 0;

		GraphicsUpscale upscale_;

		Bool verticalSync_ = false;

		Bool frameGeneration_ = false;

		Bool shapeVisible_ = false;
	};

	class SEEDCORE_API Graphics
	{
	public:
		Graphics();

		~Graphics();

		Bool Initialize(const Window& window, const SplashSystem& splashSystem, const GameConfig& gameConfig);

		void Finalize();

		void Wait();

	public:
		void Configure(const GraphicsSettings& settings);

		void ResizeSwapChain(Uint32 width, Uint32 height);

	public:
		void Begin();

		void Bind();

		void End();

	public:
		void Update(Float deltaTime, World& world);

	public:
		void EditorRender(WorldTimer& timer, const EditorCamera& editorCamera, World& world, RenderViewMode viewMode, RenderDebugSource debugSource, Uint64 showFlags);

		void GameRender(GameTimer& timer, CameraSystem& cameraSystem, World& world, RenderViewOutput output, const Vector4& outputRect);

		void CanvasRender(WorldTimer& timer, const CanvasCamera& canvasCamera, World& world);

		void PreviewRender(PreviewSlot slot, const PreviewRequest& request, const PreviewCamera& camera, Float totalTime);

		void PreviewRender(const BootRequest& request, Uint32 width, Uint32 height);

	public:
		void DrawSplashScreen(const SplashSystem& splashSystem);

	public:
		NODISCARD RenderDevice& Device()const;

		NODISCARD GraphicsModule& Module()const;

		NODISCARD const GpuProfiler& Profiler()const;

	public:
		NODISCARD const GraphicsDisplay& Display()const;

		NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE PreviewDisplay(PreviewSlot slot)const;

	private:
		struct Impl;

		ResourcePtr<Impl> impl_;
	};
}
