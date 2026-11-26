#pragma once

namespace SeedCore
{
	class SceneSystem;

	class SEEDCORE_API Scene
	{
	public:
		static void Change(const std::filesystem::path& targetScene);

		static void Change(Uint32 targetScene);

		static void Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene);

		static void Change(Uint32 targetScene, Uint32 loadingScene);

		static void Change(const std::filesystem::path& targetScene, Float fadeOutDuration, Float fadeInDuration);

		static void Change(Uint32 targetScene, Float fadeOutDuration, Float fadeInDuration);

		static void Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene, Float coverDuration, Float revealDuration);

		static void Change(Uint32 targetScene, Uint32 loadingScene, Float coverDuration, Float revealDuration);

		static Float FadeAlpha();

		static Uint32 AssetID(const String& path);

		NODISCARD static const String& CurrentName();

	private:
		friend class SceneSystem;

	private:
		static SceneSystem* system_;
	};
}
