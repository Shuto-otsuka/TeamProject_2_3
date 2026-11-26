#pragma once
#include <FoundationEngine/Resource/Scene/SceneBlueprint.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>

namespace SeedCore
{
	class World;
	class ResourceCache;
	class JobExecutor;
	class Actor;

	class SceneTransitionSystem
	{
	public:
		Bool LoadScene(World& world, ResourceCache& cache, const std::filesystem::path& targetScene);

		Bool LoadScene(World& world, ResourceCache& cache, Uint32 targetScene);

		void LoadScene(World& world, ResourceCache& cache, JobExecutor& executor, const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene);

		void LoadScene(World& world, ResourceCache& cache, JobExecutor& executor, Uint32 targetScene, Uint32 loadingScene);

		void LoadScene(World& world, ResourceCache& cache, JobExecutor& executor, const std::filesystem::path& targetScene, Float fadeOutDuration, Float fadeInDuration);

		void LoadScene(World& world, ResourceCache& cache, JobExecutor& executor, Uint32 targetScene, Float fadeOutDuration, Float fadeInDuration);

		void LoadScene(World& world, ResourceCache& cache, JobExecutor& executor, const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene, Float coverDuration, Float revealDuration);

		void LoadScene(World& world, ResourceCache& cache, JobExecutor& executor, Uint32 targetScene, Uint32 loadingScene, Float coverDuration, Float revealDuration);

		void Update(World& world, ResourceCache& cache, Float deltaTime);

		Bool Transitioning()const;

		void Reset();

		Float GetFadeAlpha()const;

		NODISCARD const SceneBlueprint* ConsumeSwitchedScene();

	private:
		enum class State
		{
			Idle,

			WaitingBackground,

			FadingOut,

			FadingIn,

			CoveringLoad,

			WaitingLoading,

			Revealing,
		};

		void BackgroundLoad(ResourceCache& cache, JobExecutor& executor, const std::filesystem::path& path);

		void SwapLoad(World& world, ResourceCache& cache);

	private:
		State state_ = State::Idle;

		SceneBlueprint pendingScene_;

		JobTaskflow pendingFlow_;

		JobFuture<void> pendingLoad_;

		Bool pendingLoadSucceeded_ = false;

		Bool sceneSwitched_ = false;

		Float fadeAlpha_ = 0.0f;

		Float fadeTimer_ = 0.0f;

		Float fadeOutDuration_ = 0.3f;

		Float fadeInDuration_ = 0.3f;

		DynamicArray<Actor> previousActors_;

		DynamicArray<Actor> loadingSceneActors_;

		Float transitionTimer_ = 0.0f;

		Float coverDuration_ = 0.3f;

		Float revealDuration_ = 0.3f;
	};
}
