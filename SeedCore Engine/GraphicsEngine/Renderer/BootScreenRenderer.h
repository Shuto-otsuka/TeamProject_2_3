#pragma once
#include <FoundationEngine/Resource/Config/BootConfig.h>
#include <GraphicsEngine/D3D12/Descriptor/DescriptorHeap.h>
#include <GraphicsEngine/D3D12/Buffer/FrameBuffer.h>
#include <GraphicsEngine/Shape/Screen/BootScreen.h>

namespace SeedCore
{
	class BindlessHeap;
	class D3D12CommandQueue;
	class D3D12CommandList;

	class SEEDCORE_API BootScreenRenderer :public NonCopyable
	{
	public:
		BootScreenRenderer() = default;
		~BootScreenRenderer() = default;

		void Create(ID3D12Device* device, D3D12CommandQueue* cmdQueue, BindlessHeap* bindlessHeap, Uint32 width, Uint32 height);

		void Resize(Uint32 width, Uint32 height);

		void LoadImages(const BootConfig& config);

		void Render(D3D12CommandList* cmdList, const BootConfig& config, Float progress, Float time);

		NODISCARD Bool Created()const;

		NODISCARD Uint32 Width()const;

		NODISCARD Uint32 Height()const;

		NODISCARD Float BarAspect()const;

		NODISCARD D3D12_GPU_DESCRIPTOR_HANDLE DisplayGPUHandle()const;

	private:
		BootScreen bootScreen_;

		DescriptorHeap renderTargetViewHeap_;
		ResourcePtr<FrameBuffer> frameBuffer_;

		ID3D12Device* device_ = nullptr;
		BindlessHeap* bindlessHeap_ = nullptr;

		Uint32 width_ = 0;
		Uint32 height_ = 0;
	};
}
