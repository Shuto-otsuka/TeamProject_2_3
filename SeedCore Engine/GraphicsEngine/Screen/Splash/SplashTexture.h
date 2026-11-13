#pragma once
#include <GraphicsEngine/System/SplashSystem.h>

namespace SeedCore
{
	class RenderDevice;

	class SEEDCORE_API SplashTexture :public NonCopyable
	{
	public:
		SplashTexture() = default;

		~SplashTexture() = default;

		void Initialize(RenderDevice& device);

		void Release();

	public:
		NODISCARD Uint32 Index(SplashPhase phase)const;

		NODISCARD Float Aspect(SplashPhase phase)const;

	private:
		struct Image
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

			Uint32 index_ = SC_INVALID;
		};

	private:
		void Load(Image& image, const Char* fileName);

		void Free(Image& image);

		NODISCARD const Image& Select(SplashPhase phase)const;

	private:
		RenderDevice* device_ = nullptr;

		Image day_;

		Image night_;

		Image warning_;

		Image fiction_;

		Image cri_;
	};
}
