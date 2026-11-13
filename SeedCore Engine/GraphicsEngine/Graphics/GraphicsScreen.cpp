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
	/**
	* [EN]
	* Keeps the device. Nothing touches the GPU until Initialize.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* デバイスを受け取る。Initialize まで GPU には触らない。
	*/
	GraphicsScreen::GraphicsScreen(RenderDevice& device) :device_(device)
	{
		/// No Code
	}

	GraphicsScreen::~GraphicsScreen() = default;

	/**
	* [EN]
	* Creates the renderer, loads the splash images and the boot screen's images from config, and takes the back buffer's render target view slot.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* レンダラーを作り、スプラッシュの絵と、config からブート画面の絵を読み込み、バックバッファの RTV の枠を確保する。
	*/
	void GraphicsScreen::Initialize(const BootConfig& config)
	{
		renderer_ = MakePtr<SceneRenderer>(device_);
		renderer_->Initialize(RenderFeatureSet::Screen, 1ull * 1024 * 1024);

		splash_.Initialize(device_);
		boot_.Initialize(device_);
		boot_.Load(config);

		screenView_ = device_.RenderTargetHeap().AllocateIndex();
	}

	/**
	* [EN]
	* Releases the images, the render target view slot and the renderer; the device must be idle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 絵、RTV の枠、レンダラーを解放する。デバイスは待機済みであること。
	*/
	void GraphicsScreen::Finalize()
	{
		splash_.Release();
		boot_.Release();

		/// [EN] The back buffer belongs to the swap chain, so only the slot goes back to the heap.
		/// [JP] バックバッファの持ち主はスワップチェインなので、ヒープへは枠だけを返す。
		if (screenView_ != SC_INVALID)
		{
			device_.RenderTargetHeap().Release(nullptr, screenView_);
			screenView_ = SC_INVALID;
		}

		renderer_.reset();
	}

	/**
	* [EN]
	* Draws splashSystem's current phase into screen: the boot screen during the boot phase, the splash image otherwise.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* splashSystem の今のフェーズを screen へ描く。ブートのフェーズならブート画面、それ以外はスプラッシュの絵。
	*/
	void GraphicsScreen::Render(ID3D12Resource* screen, const SplashSystem& splashSystem)
	{
		D3D12_RESOURCE_DESC screenDesc = screen->GetDesc();
		Uint32 width = static_cast<Uint32>(screenDesc.Width);
		Uint32 height = screenDesc.Height;

		/// [EN] One view covering the whole back buffer; the screens read only its size.
		/// [JP] バックバッファ全体を覆うビューを 1 つ。画面の Feature はその大きさだけを読む。
		RenderView view;
		view.kind_ = RenderViewKind::Game;
		view.output_ = RenderViewOutput::Screen;
		view.renderWidth_ = width;
		view.renderHeight_ = height;
		view.displayWidth_ = width;
		view.displayHeight_ = height;
		view.outputRect_ = Vector4(0.0f, 0.0f, static_cast<Float>(width), static_cast<Float>(height));

		/// [EN] The back buffer is imported as the output of both screens; only the one the phase asks for draws into it.
		/// [JP] バックバッファを両方の画面の出力として取り込む。描くのはフェーズが求めるほうだけ。
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

		/// [EN] The boot phase shows the configurable loading screen; earlier phases show the fixed splash images. The scene keeps only pointers, so both requests live until Render returns.
		/// [JP] ブートのフェーズでは設定できるローディング画面を、それより前のフェーズでは固定のスプラッシュの絵を出す。シーンはポインタだけを持つので、両方の求めは Render が戻るまで生かしておく。
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
