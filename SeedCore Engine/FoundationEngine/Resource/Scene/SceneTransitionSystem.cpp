#include <FoundationEngine/Resource/Scene/SceneTransitionSystem.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/Math/Algorithm.h>

namespace SeedCore
{
	Bool SceneTransitionSystem::LoadScene(World& world, ResourceCache& cache, const std::filesystem::path& targetScene)
	{
		std::filesystem::path resolvedPath = targetScene;
		Uint32 assetID = cache.Find(String(targetScene.string()));
		if (assetID != 0)
		{
			AssetRecord* asset = cache.Record(assetID);
			if (asset)
			{
				resolvedPath = std::filesystem::path(asset->fullpath_.c_str());
			}
		}

		if (!pendingScene_.Read(resolvedPath))
		{
			return false;
		}

		WorldAccessor::Clear(world);
		pendingScene_.Instantiate(world, cache);
		sceneSwitched_ = true;

		return true;
	}

	Bool SceneTransitionSystem::LoadScene(World& world, ResourceCache& cache, Uint32 targetScene)
	{
		AssetRecord* targetAsset = cache.Record(targetScene);
		if (!targetAsset)
		{
			return false;
		}

		return LoadScene(world, cache, std::filesystem::path(targetAsset->fullpath_.c_str()));
	}

	void SceneTransitionSystem::LoadScene(World& world, ResourceCache& cache, JobExecutor& executor, const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene)
	{
		std::filesystem::path resolvedLoadingScene = loadingScene;
		Uint32 loadingSceneAssetID = cache.Find(String(loadingScene.string()));
		if (loadingSceneAssetID != 0)
		{
			AssetRecord* loadingSceneAsset = cache.Record(loadingSceneAssetID);
			if (loadingSceneAsset)
			{
				resolvedLoadingScene = std::filesystem::path(loadingSceneAsset->fullpath_.c_str());
			}
		}

		SceneBlueprint loadingSceneBlueprint;
		if (loadingSceneBlueprint.Read(resolvedLoadingScene))
		{
			WorldAccessor::Clear(world);
			loadingSceneBlueprint.Instantiate(world, cache);
		}

		BackgroundLoad(cache, executor, targetScene);
		state_ = State::WaitingBackground;
	}

	void SceneTransitionSystem::LoadScene(World& world, ResourceCache& cache, JobExecutor& executor, Uint32 targetScene, Uint32 loadingScene)
	{
		AssetRecord* targetAsset = cache.Record(targetScene);
		AssetRecord* loadingAsset = cache.Record(loadingScene);
		if (!targetAsset || !loadingAsset)
		{
			return;
		}

		LoadScene(world, cache, executor, std::filesystem::path(targetAsset->fullpath_.c_str()), std::filesystem::path(loadingAsset->fullpath_.c_str()));
	}

	void SceneTransitionSystem::LoadScene(World& world, ResourceCache& cache, JobExecutor& executor, const std::filesystem::path& targetScene, Float fadeOutDuration, Float fadeInDuration)
	{
		fadeOutDuration_ = fadeOutDuration;
		fadeInDuration_ = fadeInDuration;
		fadeTimer_ = 0.0f;
		fadeAlpha_ = 0.0f;

		BackgroundLoad(cache, executor, targetScene);
		state_ = State::FadingOut;
	}

	void SceneTransitionSystem::LoadScene(World& world, ResourceCache& cache, JobExecutor& executor, Uint32 targetScene, Float fadeOutDuration, Float fadeInDuration)
	{
		AssetRecord* targetAsset = cache.Record(targetScene);
		if (!targetAsset)
		{
			return;
		}

		LoadScene(world, cache, executor, std::filesystem::path(targetAsset->fullpath_.c_str()), fadeOutDuration, fadeInDuration);
	}

	void SceneTransitionSystem::LoadScene(World& world, ResourceCache& cache, JobExecutor& executor, const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene, Float coverDuration, Float revealDuration)
	{
		coverDuration_ = coverDuration;
		revealDuration_ = revealDuration;
		transitionTimer_ = 0.0f;

		previousActors_.clear();
		ActorRegistry& actors = WorldAccessor::Actors(world);
		for (EntityID current = actors.Root(); current != EntityID{}; current = actors.Next(current))
		{
			previousActors_.push_back(WorldAccessor::Make(world, current));
		}

		std::filesystem::path resolvedLoadingScene = loadingScene;
		Uint32 loadingSceneAssetID = cache.Find(String(loadingScene.string()));
		if (loadingSceneAssetID != 0)
		{
			AssetRecord* loadingSceneAsset = cache.Record(loadingSceneAssetID);
			if (loadingSceneAsset)
			{
				resolvedLoadingScene = std::filesystem::path(loadingSceneAsset->fullpath_.c_str());
			}
		}

		SceneBlueprint loadingSceneBlueprint;
		loadingSceneActors_.clear();
		if (loadingSceneBlueprint.Read(resolvedLoadingScene))
		{
			loadingSceneActors_ = loadingSceneBlueprint.Instantiate(world, cache);
		}

		BackgroundLoad(cache, executor, targetScene);
		state_ = State::CoveringLoad;
	}

	void SceneTransitionSystem::LoadScene(World& world, ResourceCache& cache, JobExecutor& executor, Uint32 targetScene, Uint32 loadingScene, Float coverDuration, Float revealDuration)
	{
		AssetRecord* targetAsset = cache.Record(targetScene);
		AssetRecord* loadingAsset = cache.Record(loadingScene);
		if (!targetAsset || !loadingAsset)
		{
			return;
		}

		LoadScene(world, cache, executor, std::filesystem::path(targetAsset->fullpath_.c_str()), std::filesystem::path(loadingAsset->fullpath_.c_str()), coverDuration, revealDuration);
	}

	void SceneTransitionSystem::Update(World& world, ResourceCache& cache, Float deltaTime)
	{
		switch (state_)
		{
		case State::WaitingBackground:
			if (pendingLoad_.valid() && pendingLoad_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
			{
				SwapLoad(world, cache);
				state_ = State::Idle;
			}
			break;
		case State::FadingOut:
			fadeTimer_ += deltaTime;
			fadeAlpha_ = fadeOutDuration_ > 0.0f ? Clamp(fadeTimer_ / fadeOutDuration_, 0.0f, 1.0f) : 1.0f;

			if (fadeAlpha_ >= 1.0f && pendingLoad_.valid() && pendingLoad_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
			{
				SwapLoad(world, cache);
				fadeTimer_ = 0.0f;
				state_ = State::FadingIn;
			}
			break;
		case State::FadingIn:
			fadeTimer_ += deltaTime;
			fadeAlpha_ = fadeInDuration_ > 0.0f ? 1.0f - Clamp(fadeTimer_ / fadeInDuration_, 0.0f, 1.0f) : 0.0f;

			if (fadeTimer_ >= fadeInDuration_)
			{
				fadeAlpha_ = 0.0f;
				state_ = State::Idle;
			}
			break;
		case State::CoveringLoad:
			transitionTimer_ += deltaTime;

			if (transitionTimer_ >= coverDuration_)
			{
				for (Actor actor : previousActors_)
				{
					world.DestroyActor(actor);
				}
				previousActors_.clear();

				state_ = State::WaitingLoading;
			}
			break;
		case State::WaitingLoading:
			if (pendingLoad_.valid() && pendingLoad_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
			{
				if (pendingLoadSucceeded_)
				{
					pendingScene_.Instantiate(world, cache);
					sceneSwitched_ = true;
				}

				transitionTimer_ = 0.0f;
				state_ = State::Revealing;
			}
			break;
		case State::Revealing:
			transitionTimer_ += deltaTime;

			if (transitionTimer_ >= revealDuration_)
			{
				for (Actor actor : loadingSceneActors_)
				{
					world.DestroyActor(actor);
				}
				loadingSceneActors_.clear();

				state_ = State::Idle;
			}
			break;
		default:
			break;
		}
	}

	Bool SceneTransitionSystem::Transitioning()const
	{
		return state_ != State::Idle;
	}

	void SceneTransitionSystem::Reset()
	{
		if (pendingLoad_.valid())
		{
			pendingLoad_.wait();
		}
		pendingFlow_.Clear();
		pendingLoadSucceeded_ = false;
		sceneSwitched_ = false;

		previousActors_.clear();
		loadingSceneActors_.clear();

		fadeAlpha_ = 0.0f;
		fadeTimer_ = 0.0f;
		transitionTimer_ = 0.0f;

		state_ = State::Idle;
	}

	Float SceneTransitionSystem::GetFadeAlpha()const
	{
		return fadeAlpha_;
	}

	const SceneBlueprint* SceneTransitionSystem::ConsumeSwitchedScene()
	{
		if (!sceneSwitched_)
		{
			return nullptr;
		}

		sceneSwitched_ = false;
		return &pendingScene_;
	}

	void SceneTransitionSystem::BackgroundLoad(ResourceCache& cache, JobExecutor& executor, const std::filesystem::path& path)
	{
		std::filesystem::path resolvedPath = path;
		Uint32 assetID = cache.Find(String(path.string()));
		if (assetID != 0)
		{
			AssetRecord* asset = cache.Record(assetID);
			if (asset)
			{
				resolvedPath = std::filesystem::path(asset->fullpath_.c_str());
			}
		}

		pendingLoadSucceeded_ = false;

		pendingFlow_.Clear();
		pendingFlow_.emplace([this, resolvedPath]()
			{
				pendingLoadSucceeded_ = pendingScene_.Read(resolvedPath);
			});

		pendingLoad_ = executor.Run(pendingFlow_);
	}

	void SceneTransitionSystem::SwapLoad(World& world, ResourceCache& cache)
	{
		if (!pendingLoadSucceeded_)
		{
			return;
		}

		WorldAccessor::Clear(world);
		pendingScene_.Instantiate(world, cache);
		sceneSwitched_ = true;
	}
}
