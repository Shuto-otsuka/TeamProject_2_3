#pragma once
#include <GraphicsEngine/Quality/Upscale.h>
#include <GraphicsEngine/D3D12/SwapChain/GraphicsResolution.h>

namespace SeedCore
{
	struct SEEDCORE_API GameConfig
	{
		Uint32 windowWidth_ = 1920;

		Uint32 windowHeight_ = 1080;

		Bool fullscreen_ = false;

		ResolutionPreset resolution_ = ResolutionPreset::FHD;

		GraphicsUpscale upscale_ = { UpscaleMethod::Taau, UpscaleMode::Balanced };

		Bool useFrameGeneration_ = false;

		Bool vsync_ = false;

		Bool useReflex_ = false;

		Bool useDeepDVC_ = false;

		String initialScenePath_;

		Bool showSplashWarning_ = true;

		Bool showSplashFiction_ = true;

		String executableName_;

		Bool Initialize();

		void Load(const std::filesystem::path& path = "../UserProject/Assets/Config/GameConfig.scg");

		void Save(const std::filesystem::path& path = "../UserProject/Assets/Config/GameConfig.scg")const;
	};
}
