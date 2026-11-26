#include <FoundationEngine/Resource/Scene/Scene.h>
#include <FoundationEngine/Resource/Scene/SceneSystem.h>

namespace SeedCore
{
	SceneSystem* Scene::system_ = nullptr;

	void Scene::Change(const std::filesystem::path& targetScene)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene);
	}

	void Scene::Change(Uint32 targetScene)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene);
	}

	void Scene::Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene, loadingScene);
	}

	void Scene::Change(Uint32 targetScene, Uint32 loadingScene)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene, loadingScene);
	}

	void Scene::Change(const std::filesystem::path& targetScene, Float fadeOutDuration, Float fadeInDuration)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene, fadeOutDuration, fadeInDuration);
	}

	void Scene::Change(Uint32 targetScene, Float fadeOutDuration, Float fadeInDuration)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene, fadeOutDuration, fadeInDuration);
	}

	void Scene::Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene, Float coverDuration, Float revealDuration)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene, loadingScene, coverDuration, revealDuration);
	}

	void Scene::Change(Uint32 targetScene, Uint32 loadingScene, Float coverDuration, Float revealDuration)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene, loadingScene, coverDuration, revealDuration);
	}

	Float Scene::FadeAlpha()
	{
		if (!system_)
		{
			return 0.0f;
		}

		return system_->FadeAlpha();
	}

	Uint32 Scene::AssetID(const String& path)
	{
		if (!system_)
		{
			return 0;
		}

		return system_->AssetID(path);
	}

	const String& Scene::CurrentName()
	{
		static const String emptyName;

		if (!system_)
		{
			return emptyName;
		}

		return system_->CurrentName();
	}
}
