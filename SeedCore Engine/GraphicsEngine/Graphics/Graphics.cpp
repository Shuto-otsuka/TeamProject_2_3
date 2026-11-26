#include <GraphicsEngine/Graphics/Graphics.h>
#include <GraphicsEngine/Graphics/GraphicsPresenter.h>
#include <GraphicsEngine/Graphics/GraphicsPreview.h>
#include <GraphicsEngine/Graphics/GraphicsProcessor.h>
#include <GraphicsEngine/Graphics/GraphicsScene.h>
#include <GraphicsEngine/Graphics/GraphicsScreen.h>
#include <GraphicsEngine/Graphics/GraphicsUpdater.h>

#include <FoundationEngine/Log/AftermathCrashTracker.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Resource/Gateway.h>
#include <FoundationEngine/Resource/Config/GameConfig.h>

#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12DebugLayer.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>
#include <GraphicsEngine/DLSS/DlssManager.h>
#include <GraphicsEngine/Profiler/ProfilerStats.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>
#include <GraphicsEngine/Shader/ShaderCache.h>
#include <GraphicsEngine/Shader/ShaderHotReload.h>
#include <GraphicsEngine/System/SplashSystem.h>

namespace SeedCore
{
	struct Graphics::Impl
	{
		ResourcePtr<RenderDevice> device_;

		ResourcePtr<GraphicsPresenter> presenter_;

		GraphicsProcessor processor_;

		ResourcePtr<GraphicsModule> module_;

		ResourcePtr<GraphicsUpdater> updater_;

		ResourcePtr<GraphicsScene> scene_;

		ResourcePtr<GraphicsPreview> preview_;

		ResourcePtr<GraphicsScreen> screen_;
	};

	Graphics::Graphics() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	Graphics::~Graphics() = default;

	Bool Graphics::Initialize(const Window& window, const SplashSystem& splashSystem, const GameConfig& gameConfig)
	{
#if SC_DEBUG
		D3D12DebugLayer::Enable();
		AftermathCrashTracker::Enable();
#endif

		impl_->device_ = MakePtr<RenderDevice>();
		if (!impl_->device_->Initialize())
		{
			return false;
		}
		D3D12Context& context = impl_->device_->Context();
		BindlessHeap& resourceHeap = impl_->device_->ResourceHeap();

#if SC_DEBUG
		AftermathCrashTracker::Create(context.GetDevice());
#endif

		impl_->presenter_ = MakePtr<GraphicsPresenter>(*impl_->device_);
		if (!impl_->presenter_->Initialize(window))
		{
			return false;
		}

		impl_->screen_ = MakePtr<GraphicsScreen>(*impl_->device_);
		impl_->screen_->Initialize(splashSystem.Config());

		impl_->processor_.Initialize(*impl_->device_);

		impl_->module_ = MakePtr<GraphicsModule>(context, resourceHeap, impl_->processor_);
		impl_->updater_ = MakePtr<GraphicsUpdater>(*impl_->device_, *impl_->module_);

		DlssManager& dlssManager = Gateway::GetDlssManager();
		if (!dlssManager.Prepare(context.GetDevice()))
		{
			SC_LOG_WARNING("DLSSの準備に失敗しました - DLSS無効で続行します");
			return false;
		}

		impl_->scene_ = MakePtr<GraphicsScene>(*impl_->device_, *impl_->module_);
		impl_->scene_->Initialize();

		impl_->preview_ = MakePtr<GraphicsPreview>(*impl_->device_, *impl_->module_);

		dlssManager.ReflexEnable(gameConfig.useReflex_);
		dlssManager.Reflex(false);
		dlssManager.DeepDVCEnable(gameConfig.useDeepDVC_);
		dlssManager.DeepDVC(0.5f, 0.25f);

		ScResolution::ResSize outputSize = ToResSize(gameConfig.resolution_);
		GraphicsSettings settings;
		settings.outputWidth_ = static_cast<Uint32>(outputSize.Width);
		settings.outputHeight_ = static_cast<Uint32>(outputSize.Height);
		settings.upscale_ = gameConfig.upscale_;
		settings.verticalSync_ = gameConfig.vsync_;
		settings.frameGeneration_ = gameConfig.useFrameGeneration_;
		Configure(settings);

		return true;
	}

	void Graphics::Finalize()
	{
		Wait();

		if (impl_->preview_)
		{
			impl_->preview_->Finalize();
			impl_->preview_.reset();
		}

		if (impl_->scene_)
		{
			impl_->scene_->Finalize();
			impl_->scene_.reset();
		}

		if (impl_->screen_)
		{
			impl_->screen_->Finalize();
			impl_->screen_.reset();
		}

		if (impl_->presenter_)
		{
			impl_->presenter_->Finalize();
			impl_->presenter_.reset();
		}

		impl_->updater_.reset();
		impl_->module_.reset();

		impl_->processor_.Finalize();

#if SC_DEBUG
		AftermathCrashTracker::Disable();
#endif
		if (impl_->device_)
		{
			impl_->device_->ShaderCache().Clear();
			impl_->device_->ShaderHotReload().Clear();
			impl_->device_.reset();
		}

#if SC_DEBUG
		D3D12DebugLayer::Report();
#endif
	}

	void Graphics::Wait()
	{
		impl_->device_->Wait();
	}

	void Graphics::Configure(const GraphicsSettings& settings)
	{
		impl_->scene_->Configure(settings);
		impl_->updater_->Configure(settings);
		impl_->presenter_->Configure(settings);
	}

	void Graphics::ResizeSwapChain(Uint32 width, Uint32 height)
	{
		impl_->presenter_->Resize(width, height);
	}

	void Graphics::Begin()
	{
		ProfilerStats::Reset();
		impl_->scene_->Begin();
		impl_->device_->Context().BeginFrame();
		impl_->presenter_->Begin();
	}

	void Graphics::Bind()
	{
		impl_->scene_->Render(impl_->presenter_->BackBuffer(), impl_->updater_->Celestial(), impl_->updater_->Shapes());
		impl_->preview_->Render();
		impl_->presenter_->Bind();
	}

	void Graphics::End()
	{
		impl_->scene_->Render(impl_->presenter_->BackBuffer(), impl_->updater_->Celestial(), impl_->updater_->Shapes());
		impl_->preview_->Render();

		impl_->presenter_->End();
		impl_->device_->Context().EndFrame();
		impl_->presenter_->Present();

		impl_->device_->ResourceHeap().Retire();
		impl_->device_->SamplerHeap().Retire();
		impl_->device_->RenderTargetHeap().Retire();
		impl_->device_->DepthStencilHeap().Retire();
	}

	void Graphics::Update(Float deltaTime, World& world)
	{
		impl_->updater_->Update(deltaTime, world, impl_->scene_->AspectRatio());
	}

	void Graphics::EditorRender(WorldTimer& timer, const EditorCamera& editorCamera, World& world, RenderViewMode viewMode, RenderDebugSource debugSource, Uint64 showFlags)
	{
		impl_->scene_->AddEditorView(timer, editorCamera, world, viewMode, debugSource, showFlags);
	}

	void Graphics::GameRender(GameTimer& timer, CameraSystem& cameraSystem, World& world, RenderViewOutput output, const Vector4& outputRect)
	{
		impl_->scene_->AddGameView(timer, cameraSystem, world, output, outputRect);
	}

	void Graphics::CanvasRender(WorldTimer& timer, const CanvasCamera& canvasCamera, World& world)
	{
		impl_->scene_->AddCanvasView(timer, canvasCamera, world);
	}

	void Graphics::PreviewRender(PreviewSlot slot, const PreviewRequest& request, const PreviewCamera& camera, Float totalTime)
	{
		impl_->preview_->Add(slot, request, camera, totalTime);
	}

	void Graphics::PreviewRender(const BootRequest& request, Uint32 width, Uint32 height)
	{
		impl_->preview_->Add(request, width, height);
	}

	void Graphics::DrawSplashScreen(const SplashSystem& splashSystem)
	{
		Begin();
		impl_->screen_->Render(impl_->presenter_->BackBuffer(), splashSystem);

		End();
	}

	RenderDevice& Graphics::Device()const
	{
		return *impl_->device_;
	}

	GraphicsModule& Graphics::Module()const
	{
		return *impl_->module_;
	}

	const GpuProfiler& Graphics::Profiler()const
	{
		return impl_->scene_->Profiler();
	}

	const GraphicsDisplay& Graphics::Display()const
	{
		return impl_->scene_->Display();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE Graphics::PreviewDisplay(PreviewSlot slot)const
	{
		return impl_->preview_->Display(slot);
	}
}
