#include <GraphicsEngine/Graphics/GraphicsScreen.h>

#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderView.h>
#include <GraphicsEngine/RenderGraph/SceneRenderer.h>
#include <GraphicsEngine/Screen/Boot/BootFeature.h>
#include <GraphicsEngine/Screen/Boot/BootRequest.h>
#include <GraphicsEngine/Screen/Splash/SplashFeature.h>
#include <GraphicsEngine/Screen/Splash/SplashRequest.h>
#include <GraphicsEngine/System/SplashSystem.h>

namespace SeedCore
{
	GraphicsScreen::GraphicsScreen(RenderDevice& device) :device_(device)
	{
		/// No Code
	}

	GraphicsScreen::~GraphicsScreen() = default;

	void GraphicsScreen::Initialize(const BootConfig& config)
	{
		renderer_ = MakePtr<SceneRenderer>(device_);
		renderer_->Initialize(RenderFeatureSet::Screen, 1ull * 1024 * 1024);

		splash_.Initialize(device_);
		boot_.Initialize(device_);
		boot_.Load(config);

		screenView_ = device_.RenderTargetHeap().AllocateIndex();
	}

	void GraphicsScreen::Finalize()
	{
		splash_.Release();
		boot_.Release();

		if (screenView_ != SC_INVALID)
		{
			device_.RenderTargetHeap().Release(nullptr, screenView_);
			screenView_ = SC_INVALID;
		}

		renderer_.reset();
	}

	void GraphicsScreen::Render(ID3D12Resource* screen, const SplashSystem& splashSystem)
	{
		D3D12_RESOURCE_DESC screenDesc = screen->GetDesc();
		Uint32 width = static_cast<Uint32>(screenDesc.Width);
		Uint32 height = screenDesc.Height;

		RenderView view;
		view.kind_ = RenderViewKind::Game;
		view.output_ = RenderViewOutput::Screen;
		view.renderWidth_ = width;
		view.renderHeight_ = height;
		view.displayWidth_ = width;
		view.displayHeight_ = height;
		view.outputRect_ = Vector4(0.0f, 0.0f, static_cast<Float>(width), static_cast<Float>(height));

		device_.Context().GetDevice()->CreateRenderTargetView(screen, nullptr, device_.RenderTargetHeap().CPUHandle(screenView_));
		RenderGraphAllocation screenOutput;
		screenOutput.resource_ = screen;
		screenOutput.state_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
		screenOutput.format_ = static_cast<RenderFormat>(screenDesc.Format);
		screenOutput.width_ = width;
		screenOutput.height_ = height;
		screenOutput.renderTarget_ = true;
		screenOutput.renderTargetView_ = screenView_;

		DynamicArray<RenderGraphImport> imports;
		imports.push_back(RenderGraphImport{ String(SplashFeature::Output.Name()), view.kind_, screenOutput });
		imports.push_back(RenderGraphImport{ String(BootFeature::Output.Name()), view.kind_, screenOutput });

		RenderScene scene;
		scene.views_ = std::span<const RenderView>(&view, 1);
		scene.imports_ = imports;

		BootRequest bootRequest;
		SplashRequest splashRequest;
		if (splashSystem.Phase() == SplashPhase::Boot)
		{
			bootRequest.alpha_ = 1.0f;
			bootRequest.progress_ = splashSystem.Progress();
			bootRequest.time_ = splashSystem.Time();
			bootRequest.config_ = &splashSystem.Config();
			bootRequest.texture_ = &boot_;
			scene.Set(bootRequest);
		}
		else
		{
			splashRequest.phase_ = splashSystem.Phase();
			splashRequest.alpha_ = splashSystem.Alpha();
			splashRequest.texture_ = &splash_;
			scene.Set(splashRequest);
		}

		renderer_->Render(scene);
	}
}
