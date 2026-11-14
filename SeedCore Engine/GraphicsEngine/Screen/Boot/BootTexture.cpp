#include <GraphicsEngine/Screen/Boot/BootTexture.h>
#include <GraphicsEngine/Texture/TextureCreator.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	void BootTexture::Initialize(RenderDevice& device)
	{
		device_ = &device;
	}

	void BootTexture::Load(const BootConfig& config)
	{
		Replace(background_, config.backgroundImage_, "ProgressBackground");
		Replace(bar_, config.barImage_, "ProgressBar");
		Replace(frame_, config.frameImage_, "ProgressFrame");
	}

	void BootTexture::Release()
	{
		if (!device_)
		{
			return;
		}
		Free(background_);
		Free(bar_);
		Free(frame_);
	}

	Uint32 BootTexture::BackgroundIndex()const
	{
		return background_.index_;
	}

	Uint32 BootTexture::BarIndex()const
	{
		return bar_.index_;
	}

	Uint32 BootTexture::FrameIndex()const
	{
		return frame_.index_;
	}

	Float BootTexture::BackgroundAspect()const
	{
		return Aspect(background_);
	}

	Float BootTexture::BarAspect()const
	{
		return Aspect(bar_);
	}

	Vector4 BootTexture::BarRect(const BootConfig& config, Float screenWidth, Float screenHeight)const
	{
		Int32 anchorIndex = static_cast<Int32>(config.anchor_);
		Vector2 anchor = Vector2(static_cast<Float>(anchorIndex % 3) * 0.5f, static_cast<Float>(anchorIndex / 3) * 0.5f);
		Vector2 size = Vector2(config.width_, config.width_ * screenWidth / (Max(BarAspect(), 0.0001f) * Max(screenHeight, 1.0f)));
		Vector2 barMin = anchor + config.offset_ - anchor * size;
		return Vector4(barMin.x, barMin.y, barMin.x + size.x, barMin.y + size.y);
	}

	void BootTexture::Replace(Image& image, const DynamicArray<Byte>& data, const Char* defaultName)
	{
		Free(image);
		BindlessHeap& heap = device_->ResourceHeap();
		D3D12Context& context = device_->Context();
		image.index_ = heap.AllocateIndex();
		if (!data.empty())
		{
			TextureCreator::CreateTextureMemory(context.GetDevice(), context.GetCopyQueue(), context.GetLoadTransferLane(LoadThread::Main), heap.Heap(), data, image.resource_, image.index_);
		}
		if (!image.resource_)
		{
			String filePath = String(std::string("../Runtime/Logo/") + defaultName + ".sub.logo");
			TextureCreator::CreateTexturePath(context.GetDevice(), context.GetCopyQueue(), context.GetLoadTransferLane(LoadThread::Main), heap.Heap(), filePath, image.resource_, image.index_);
		}
		if (!image.resource_)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			shaderResourceViewDesc.Texture2D.MipLevels = 1;
			context.GetDevice()->CreateShaderResourceView(nullptr, &shaderResourceViewDesc, heap.CPUHandle(image.index_));
		}
	}

	void BootTexture::Free(Image& image)
	{
		if (image.index_ == SC_INVALID)
		{
			return;
		}
		device_->ResourceHeap().Release(std::move(image.resource_), image.index_);
		image.index_ = SC_INVALID;
	}

	Float BootTexture::Aspect(const Image& image)
	{
		if (!image.resource_)
		{
			return 1.0f;
		}
		D3D12_RESOURCE_DESC desc = image.resource_->GetDesc();
		return static_cast<Float>(desc.Width) / static_cast<Float>(Max<Uint32>(desc.Height, 1));
	}
}
