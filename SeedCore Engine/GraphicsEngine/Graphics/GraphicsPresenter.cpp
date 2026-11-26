#include <GraphicsEngine/Graphics/GraphicsPresenter.h>
#include <GraphicsEngine/Graphics/Graphics.h>

#include <FoundationEngine/Application/Window.h>
#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Resource/Gateway.h>

#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/SwapChain/SwapChain.h>
#include <GraphicsEngine/DLSS/DlssManager.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>

namespace SeedCore
{
	GraphicsPresenter::GraphicsPresenter(RenderDevice& device) :device_(device)
	{
		/// No Code
	}

	GraphicsPresenter::~GraphicsPresenter() = default;

	Bool GraphicsPresenter::Initialize(const Window& window)
	{
		HWND hwnd = window.Handle();

		RECT clientRect{};
		GetClientRect(hwnd, &clientRect);
		Float width = static_cast<Float>(clientRect.right - clientRect.left);
		Float height = static_cast<Float>(clientRect.bottom - clientRect.top);

		D3D12Context& context = device_.Context();

		swapChain_ = MakePtr<SwapChain>(width, height);
		if (!swapChain_->Create(context.GetFactory(), context.GetDevice(), context.GetDirectQueue()->GetCommandQueue(), hwnd))
		{
			return false;
		}

		return true;
	}

	void GraphicsPresenter::Finalize()
	{
		swapChain_.reset();
	}

	void GraphicsPresenter::Configure(const GraphicsSettings& settings)
	{
		swapChain_->VerticalSync(settings.verticalSync_);

		DlssManager& dlssManager = Gateway::GetDlssManager();
		if (dlssManager.FrameGenerationEnable() == settings.frameGeneration_)
		{
			return;
		}
		HWND hwnd = swapChain_->GetHwnd();
		device_.Wait();
		swapChain_->Destroy();
		dlssManager.FrameGenerationEnable(settings.frameGeneration_);
		D3D12Context& context = device_.Context();
		swapChain_->Create(context.GetFactory(), context.GetDevice(), context.GetDirectQueue()->GetCommandQueue(), hwnd);
	}

	void GraphicsPresenter::Resize(Uint32 width, Uint32 height)
	{
		DlssManager& dlssManager = Gateway::GetDlssManager();
		dlssManager.FrameGenerationSuppress(true);

		device_.Wait();

		swapChain_->Resize(device_.Context().GetDevice(), static_cast<Float>(width), static_cast<Float>(height));

		dlssManager.FrameGenerationSuppress(false);
	}

	void GraphicsPresenter::Begin()
	{
		DlssManager& dlssManager = Gateway::GetDlssManager();
		dlssManager.BeginFrame();
		dlssManager.EvaluateReflex();
		dlssManager.EvaluateFrameGeneration(1);

		device_.Context().GetDirectList()->Barrier(swapChain_->BackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	void GraphicsPresenter::Bind()
	{
		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = swapChain_->Handle();
		device_.Context().GetDirectList()->Get()->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, nullptr);
	}

	void GraphicsPresenter::End()
	{
		device_.Context().GetDirectList()->Barrier(swapChain_->BackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}

	void GraphicsPresenter::Present()
	{
		Benchmark::Begin("Present");
		swapChain_->Present(device_.Context().GetDevice());
		Benchmark::End("Present");
	}

	ID3D12Resource* GraphicsPresenter::BackBuffer()const
	{
		return swapChain_->BackBuffer();
	}
}
