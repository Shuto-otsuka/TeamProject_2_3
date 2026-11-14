#pragma once
#include <FoundationEngine/Resource/Config/BootConfig.h>

namespace SeedCore
{
	class RenderDevice;

	class SEEDCORE_API BootTexture :public NonCopyable
	{
	public:
		BootTexture() = default;

		~BootTexture() = default;

		void Initialize(RenderDevice& device);

		void Load(const BootConfig& config);

		void Release();

	public:
		NODISCARD Uint32 BackgroundIndex()const;

		NODISCARD Uint32 BarIndex()const;

		NODISCARD Uint32 FrameIndex()const;

		NODISCARD Float BackgroundAspect()const;

		NODISCARD Float BarAspect()const;

		NODISCARD Vector4 BarRect(const BootConfig& config, Float screenWidth, Float screenHeight)const;

	private:
		struct Image
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

			Uint32 index_ = SC_INVALID;
		};

	private:
		void Replace(Image& image, const DynamicArray<Byte>& data, const Char* defaultName);

		void Free(Image& image);

		NODISCARD static Float Aspect(const Image& image);

	private:
		RenderDevice* device_ = nullptr;

		Image background_;

		Image bar_;

		Image frame_;
	};
}
