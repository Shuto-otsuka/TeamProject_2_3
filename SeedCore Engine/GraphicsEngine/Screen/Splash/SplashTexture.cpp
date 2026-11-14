#include <GraphicsEngine/Screen/Splash/SplashTexture.h>
#include <GraphicsEngine/Texture/TextureCreator.h>
#include <GraphicsEngine/D3D12/Context/D3D12Context.h>
#include <GraphicsEngine/D3D12/Descriptor/BindlessHeap.h>
#include <GraphicsEngine/RenderGraph/RenderDevice.h>
#include <FoundationEngine/Resource/Asset/Asset.h>

namespace SeedCore
{
	void SplashTexture::Initialize(RenderDevice& device)
	{
		device_ = &device;
		Load(day_, "Day.logo");
		Load(night_, "Night.logo");
		Load(warning_, "Warning.sub.logo");
		Load(fiction_, "Fiction.sub.logo");
		Load(cri_, "CriWare.logo");
	}

	void SplashTexture::Release()
	{
		if (!device_)
		{
			return;
		}
		Free(day_);
		Free(night_);
		Free(warning_);
		Free(fiction_);
		Free(cri_);
	}

	Uint32 SplashTexture::Index(SplashPhase phase)const
	{
		return Select(phase).index_;
	}

	Float SplashTexture::Aspect(SplashPhase phase)const
	{
		const Image& image = Select(phase);
		if (!image.resource_)
		{
			return 1.0f;
		}
		D3D12_RESOURCE_DESC desc = image.resource_->GetDesc();
		return static_cast<Float>(desc.Width) / static_cast<Float>(Max<Uint32>(desc.Height, 1));
	}

	void SplashTexture::Load(Image& image, const Char* fileName)
	{
		BindlessHeap& heap = device_->ResourceHeap();
		D3D12Context& context = device_->Context();
		image.index_ = heap.AllocateIndex();
		String filePath = String(std::string("../Runtime/Logo/") + fileName);
		TextureCreator::CreateTexturePath(context.GetDevice(), context.GetCopyQueue(), context.GetLoadTransferLane(LoadThread::Main), heap.Heap(), filePath, image.resource_, image.index_);
	}

	void SplashTexture::Free(Image& image)
	{
		if (image.index_ == SC_INVALID)
		{
			return;
		}
		device_->ResourceHeap().Release(std::move(image.resource_), image.index_);
		image.index_ = SC_INVALID;
	}

	const SplashTexture::Image& SplashTexture::Select(SplashPhase phase)const
	{
		if (phase == SplashPhase::Warning)
		{
			return warning_;
		}
		if (phase == SplashPhase::Fiction)
		{
			return fiction_;
		}
		if (phase == SplashPhase::CriLogo)
		{
			return cri_;
		}
		std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm local{};
		localtime_s(&local, &now);
		return local.tm_hour >= 6 && local.tm_hour < 18 ? day_ : night_;
	}
}
