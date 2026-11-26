#include <GraphicsEngine/Graphics/GraphicsScene.h>
#include <GraphicsEngine/Graphics/Graphics.h>

#include <GraphicsEngine/Camera/CanvasCamera.h>
#include <GraphicsEngine/Camera/EditorCamera.h>
#include <GraphicsEngine/System/CameraSystem.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderGraphMemory.h>
#include <GraphicsEngine/RenderGraph/SceneRenderer.h>
#include <GraphicsEngine/Shape/HUD/HUDFeature.h>
#include <GraphicsEngine/Resource/GraphicsModule.h>

#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Interop/ShapeInstance.h>
#include <FoundationEngine/Time/GameTimer.h>
#include <FoundationEngine/Time/WorldTimer.h>

namespace SeedCore
{
	GraphicsScene::GraphicsScene(RenderDevice& device, GraphicsModule& module) :device_(device), module_(module)
	{
		/// No Code
	}

	GraphicsScene::~GraphicsScene() = default;

	void GraphicsScene::Initialize()
	{
		renderer_ = MakePtr<SceneRenderer>(device_);
		renderer_->Initialize(RenderFeatureSet::Scene, 64ull * 1024 * 1024);

		screenView_ = device_.RenderTargetHeap().AllocateIndex();
	}

	void GraphicsScene::Finalize()
	{
		for (RenderGraphAllocation& allocation : outputs_)
		{
			RenderGraphMemory::Release(device_, allocation);
		}

		device_.RenderTargetHeap().Release(nullptr, screenView_);
		screenView_ = SC_INVALID;

		renderer_.reset();
	}

	void GraphicsScene::Configure(const GraphicsSettings& settings)
	{
		Float scale = UpscaleRenderScale(settings.upscale_.mode_);
		nativeWidth_ = Max<Uint32>(64, static_cast<Uint32>(settings.outputWidth_ * scale + 0.5f));
		nativeHeight_ = Max<Uint32>(64, static_cast<Uint32>(settings.outputHeight_ * scale + 0.5f));
		outputWidth_ = settings.outputWidth_;
		outputHeight_ = settings.outputHeight_;
		upscale_ = settings.upscale_;
	}

	void GraphicsScene::Begin()
	{
		rendered_ = false;
		viewCount_ = 0;
		world_ = nullptr;
	}

	void GraphicsScene::AddEditorView(WorldTimer& timer, const EditorCamera& editorCamera, World& world, RenderViewMode viewMode, RenderDebugSource debugSource, Uint64 showFlags)
	{
		if (viewCount_ >= RenderViewCount)
		{
			return;
		}

		RenderView& view = views_[viewCount_];
		++viewCount_;
		world_ = &world;

		view.kind_ = RenderViewKind::Editor;
		view.mode_ = viewMode;
		view.debugSource_ = debugSource;
		view.showFlags_ = showFlags;
		view.output_ = RenderViewOutput::Texture;
		view.renderWidth_ = nativeWidth_;
		view.renderHeight_ = nativeHeight_;
		view.displayWidth_ = outputWidth_;
		view.displayHeight_ = outputHeight_;
		view.outputRect_ = Vector4(0.0f, 0.0f, static_cast<Float>(outputWidth_), static_cast<Float>(outputHeight_));
		view.hasActiveCamera_ = true;

		SceneConstantBuffer& scene = view.scene_;
		scene = SceneConstantBuffer{};
		scene.view_ = editorCamera.View();
		scene.inverseView_ = editorCamera.InverseView();
		scene.projection_ = editorCamera.Projection();
		scene.inverseProjection_ = editorCamera.InverseProjection();
		scene.nonJitterProjection_ = editorCamera.NonJitterProjection();
		scene.currentViewProjection_ = editorCamera.CurrentViewProjection();
		scene.previousViewProjection_ = editorCamera.PreviousViewProjection();
		scene.inverseViewProjection_ = editorCamera.InverseViewProjection();
		scene.nonJitterViewProjection_ = editorCamera.NonJitterViewProjection();
		scene.previousNonJitterViewProjection_ = editorCamera.PreviousNonJitterViewProjection();

		scene.cameraPosition_ = Vector4(editorCamera.Eye(), 1.0f);
		scene.cameraFocus_ = Vector4(editorCamera.Focus(), 1.0f);
		scene.fieldOfView_ = editorCamera.Fov();
		scene.nearPlane_ = editorCamera.Near();
		scene.farPlane_ = editorCamera.Far();
		scene.viewMode_ = static_cast<Uint>(viewMode);
		scene.showFlagsLow_ = static_cast<Uint>(showFlags);
		scene.showFlagsHigh_ = static_cast<Uint>(showFlags >> 32);
		scene.debugSource_ = static_cast<Uint>(debugSource);
		scene.viewKind_ = static_cast<Uint>(view.kind_);
		scene.totalTime_ = timer.TotalTime();
		scene.deltaTime_ = timer.DeltaTime();
		scene.screenSize_ = Vector2(static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		scene.inverseScreenSize_ = Vector2(1.0f / nativeWidth_, 1.0f / nativeHeight_);
		scene.displaySize_ = Vector2(static_cast<Float>(outputWidth_), static_cast<Float>(outputHeight_));
	}

	void GraphicsScene::AddGameView(GameTimer& timer, CameraSystem& cameraSystem, World& world, RenderViewOutput output, const Vector4& outputRect)
	{
		cameraSystem.Update(world, timer, static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));

		if (viewCount_ >= RenderViewCount)
		{
			return;
		}

		RenderView& view = views_[viewCount_];
		++viewCount_;
		world_ = &world;

		view.kind_ = RenderViewKind::Game;
		view.mode_ = RenderViewMode::Lit;
		view.debugSource_ = RenderDebugSource::Reflection;
		view.showFlags_ = static_cast<Uint64>(RenderShowFlag::All);
		view.output_ = output;
		view.renderWidth_ = nativeWidth_;
		view.renderHeight_ = nativeHeight_;
		view.displayWidth_ = outputWidth_;
		view.displayHeight_ = outputHeight_;
		view.outputRect_ = outputRect;
		view.hasActiveCamera_ = cameraSystem.ActiveCamera();

		view.scene_ = cameraSystem.GetSceneConstantBuffer();
		view.scene_.displaySize_ = Vector2(static_cast<Float>(outputWidth_), static_cast<Float>(outputHeight_));
		view.scene_.viewMode_ = static_cast<Uint>(RenderViewMode::Lit);
		view.scene_.showFlagsLow_ = static_cast<Uint>(view.showFlags_);
		view.scene_.showFlagsHigh_ = static_cast<Uint>(view.showFlags_ >> 32);
		view.scene_.debugSource_ = static_cast<Uint>(view.debugSource_);
		view.scene_.viewKind_ = static_cast<Uint>(view.kind_);
	}

	void GraphicsScene::AddCanvasView(WorldTimer& timer, const CanvasCamera& canvasCamera, World& world)
	{
		if (viewCount_ >= RenderViewCount)
		{
			return;
		}

		RenderView& view = views_[viewCount_];
		++viewCount_;
		world_ = &world;

		view.kind_ = RenderViewKind::Canvas;
		view.mode_ = RenderViewMode::Lit;
		view.debugSource_ = RenderDebugSource::Reflection;
		view.showFlags_ = static_cast<Uint64>(RenderShowFlag::All);
		view.output_ = RenderViewOutput::Texture;
		view.renderWidth_ = nativeWidth_;
		view.renderHeight_ = nativeHeight_;
		view.displayWidth_ = nativeWidth_;
		view.displayHeight_ = nativeHeight_;
		view.outputRect_ = Vector4(0.0f, 0.0f, static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		view.hasActiveCamera_ = true;

		SceneConstantBuffer& scene = view.scene_;
		scene = SceneConstantBuffer{};
		scene.view_ = canvasCamera.View();
		scene.inverseView_ = canvasCamera.InverseView();
		scene.projection_ = canvasCamera.Projection();
		scene.inverseProjection_ = canvasCamera.InverseProjection();
		scene.nonJitterProjection_ = canvasCamera.NonJitterProjection();
		scene.currentViewProjection_ = canvasCamera.CurrentViewProjection();
		scene.previousViewProjection_ = canvasCamera.PreviousViewProjection();
		scene.inverseViewProjection_ = canvasCamera.InverseViewProjection();
		scene.nonJitterViewProjection_ = canvasCamera.NonJitterViewProjection();
		scene.previousNonJitterViewProjection_ = canvasCamera.PreviousViewProjection();

		scene.cameraPosition_ = Vector4(canvasCamera.Eye(), 1.0f);
		scene.cameraFocus_ = Vector4(canvasCamera.Focus(), 1.0f);
		scene.fieldOfView_ = canvasCamera.Fov();
		scene.nearPlane_ = canvasCamera.Near();
		scene.farPlane_ = canvasCamera.Far();
		scene.totalTime_ = timer.TotalTime();
		scene.deltaTime_ = timer.DeltaTime();
		scene.screenSize_ = Vector2(static_cast<Float>(nativeWidth_), static_cast<Float>(nativeHeight_));
		scene.inverseScreenSize_ = Vector2(1.0f / nativeWidth_, 1.0f / nativeHeight_);
		scene.displaySize_ = scene.screenSize_;
		scene.showFlagsLow_ = static_cast<Uint>(view.showFlags_);
		scene.showFlagsHigh_ = static_cast<Uint>(view.showFlags_ >> 32);
		scene.debugSource_ = static_cast<Uint>(view.debugSource_);
		scene.viewKind_ = static_cast<Uint>(view.kind_);
	}

	void GraphicsScene::Render(ID3D12Resource* screen, const CelestialResult& celestial, const ShapeList& shapes)
	{
		if (rendered_ || viewCount_ == 0 || !world_)
		{
			return;
		}
		rendered_ = true;

		Benchmark::Begin("Render");

		D3D12CommandList& cmdList = *device_.Context().GetDirectList();
		DynamicArray<RenderGraphImport> imports;
		for (Uint32 viewIndex = 0; viewIndex < viewCount_; ++viewIndex)
		{
			const RenderView& view = views_[viewIndex];

			if (view.output_ == RenderViewOutput::Screen)
			{
				D3D12_RESOURCE_DESC screenDesc = screen->GetDesc();
				device_.Context().GetDevice()->CreateRenderTargetView(screen, nullptr, device_.RenderTargetHeap().CPUHandle(screenView_));

				RenderGraphAllocation screenOutput;
				screenOutput.resource_ = screen;
				screenOutput.state_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
				screenOutput.format_ = static_cast<RenderFormat>(screenDesc.Format);
				screenOutput.width_ = static_cast<Uint32>(screenDesc.Width);
				screenOutput.height_ = screenDesc.Height;
				screenOutput.renderTarget_ = true;
				screenOutput.renderTargetView_ = screenView_;
				imports.push_back(RenderGraphImport{ String(HUDFeature::Output.Name()), view.kind_, screenOutput });
				continue;
			}

			RenderGraphAllocation& output = outputs_[static_cast<Size>(view.kind_)];
			if (!output.resource_ || output.width_ != view.displayWidth_ || output.height_ != view.displayHeight_)
			{
				RenderGraphMemory::Release(device_, output);

				RenderGraphResource resource;
				resource.resourceName_ = String(HUDFeature::Output.Name());
				resource.desc_.shape_ = RenderResourceShape::Texture2D;
				resource.desc_.scale_ = RenderResourceScale::Fixed;
				resource.desc_.format_ = RenderFormat::RGBA8Unorm;
				resource.shaderResource_ = true;
				resource.unorderedAccess_ = true;
				resource.renderTarget_ = true;
				RenderGraphInstance instance;
				instance.width_ = view.displayWidth_;
				instance.height_ = view.displayHeight_;
				output = RenderGraphMemory::Create(device_, resource, instance, { 0.0f, 0.0f, 0.0f, 1.0f });

				StaticArray<Float, 4> black = { 0.0f, 0.0f, 0.0f, 1.0f };
				cmdList.Barrier(output.resource_.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
				cmdList.Get()->ClearRenderTargetView(device_.RenderTargetHeap().CPUHandle(output.renderTargetView_), black.data(), 0, nullptr);
				cmdList.Barrier(output.resource_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				output.state_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}
			imports.push_back(RenderGraphImport{ String(HUDFeature::Output.Name()), view.kind_, output });
			display_.views_[static_cast<Size>(view.kind_)] = device_.ResourceHeap().GPUHandle(output.shaderResourceView_);
		}

		RenderScene scene;
		scene.world_ = world_;
		scene.resource_ = &module_.Resources();
		scene.Set(celestial);
		scene.Set(upscale_);
		scene.Set(shapes);
		scene.views_ = std::span<const RenderView>(views_.data(), viewCount_);
		scene.imports_ = imports;
		renderer_->Render(scene);

		Benchmark::End("Render");
	}

	Float GraphicsScene::AspectRatio()const
	{
		return static_cast<Float>(nativeWidth_) / static_cast<Float>(nativeHeight_);
	}

	const GpuProfiler& GraphicsScene::Profiler()const
	{
		return renderer_->Profiler();
	}

	const GraphicsDisplay& GraphicsScene::Display()const
	{
		return display_;
	}
}
