#pragma once
#include <GraphicsEngine/Screen/Boot/BootTexture.h>
#include <GraphicsEngine/Screen/Splash/SplashTexture.h>

namespace SeedCore
{
	class RenderDevice;
	class SceneRenderer;
	class SplashSystem;

	class GraphicsScreen
	{
	public:
		explicit GraphicsScreen(RenderDevice& device);

		~GraphicsScreen();

		void Initialize(const BootConfig& config);

		void Finalize();

	public:
		void Render(ID3D12Resource* screen, const SplashSystem& splashSystem);

	private:
		RenderDevice& device_;

		ResourcePtr<SceneRenderer> renderer_;

		SplashTexture splash_;

		BootTexture boot_;

		Uint32 screenView_ = SC_INVALID;
	};
}
