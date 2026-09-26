#include <GraphicsEngine/Renderer/BootScreenRenderer.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandList.h>
#include <GraphicsEngine/D3D12/Context/D3D12CommandQueue.h>

namespace SeedCore
{
	void BootScreenRenderer::Create(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BindlessHeap* bindlessHeap, Uint32 width, Uint32 height)
	{
		device_ = device;
		bindlessHeap_ = bindlessHeap;
		width_ = Max<Uint32>(width, 1);
		height_ = Max<Uint32>(height, 1);

		bootScreen_.Initialize(device, cmdQueue, bindlessHeap, DXGI_FORMAT_R8G8B8A8_UNORM);

		renderTargetViewHeap_.Create(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
		frameBuffer_ = MakePtr<FrameBuffer>(device, &renderTargetViewHeap_, bindlessHeap, width_, height_, DXGI_FORMAT_R8G8B8A8_UNORM, nullptr, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	void BootScreenRenderer::Resize(Uint32 width, Uint32 height)
	{
		width = Max<Uint32>(width, 1);
		height = Max<Uint32>(height, 1);
		if (!frameBuffer_ || (width == width_ && height == height_))
		{
			return;
		}

		width_ = width;
		height_ = height;

		renderTargetViewHeap_.Create(device_, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
		frameBuffer_->Resize(device_, bindlessHeap_, width_, height_);
	}

	void BootScreenRenderer::LoadImages(const BootConfig& config)
	{
		bootScreen_.LoadImages(config);
	}

	void BootScreenRenderer::Render(D3D12CommandList* cmdList, const BootConfig& config, Float progress, Float time)
	{
		if (!frameBuffer_)
		{
			return;
		}

		frameBuffer_->Begin(cmdList);
		frameBuffer_->Clear(cmdList, 0.0f, 0.0f, 0.0f, 1.0f);
		bootScreen_.Draw(cmdList->Get(), frameBuffer_->RenderTargetViewHandle(), config, static_cast<Float>(width_), static_cast<Float>(height_), progress, time, 1.0f);
		frameBuffer_->End(cmdList);
	}

	Bool BootScreenRenderer::Created()const
	{
		return frameBuffer_ != nullptr;
	}

	Uint32 BootScreenRenderer::Width()const
	{
		return width_;
	}

	Uint32 BootScreenRenderer::Height()const
	{
		return height_;
	}

	Float BootScreenRenderer::BarAspect()const
	{
		return bootScreen_.BarAspect();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE BootScreenRenderer::DisplayGPUHandle()const
	{
		return bindlessHeap_->GPUHandle(frameBuffer_->ColorShaderResourceViewIndex());
	}
}
