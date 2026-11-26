#pragma once
#include <FoundationEngine/Resource/Scene/SceneBlueprint.h>
#include <FoundationEngine/Resource/Scene/SceneTransitionSystem.h>
#include <FoundationEngine/Utility/NonTransferable.h>

namespace SeedCore
{
	class World;
	class ResourceCache;
	class JobExecutor;
	struct GameConfig;
	struct EditorConfig;

	class SEEDCORE_API SceneSystem :public NonTransferable
	{
	public:
		SceneSystem() = default;

		~SceneSystem();

		Bool Initialize(World& world, ResourceCache& cache, JobExecutor& executor, const GameConfig& gameConfig);

		Bool Initialize(World& world, ResourceCache& cache, JobExecutor& executor, const EditorConfig& editorConfig);

	public:
		Bool Save(const std::filesystem::path& path);

		Bool Load(const std::filesystem::path& path);

		Bool Load(Uint32 assetID);

		void Capture(SceneBlueprint& blueprint);

		void Restore(const SceneBlueprint& blueprint);

	public:
		void Update(Float deltaTime);

		void Reset();

		void Change(const std::filesystem::path& targetScene);

		void Change(Uint32 targetScene);

		void Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene);

		void Change(Uint32 targetScene, Uint32 loadingScene);

		void Change(const std::filesystem::path& targetScene, Float fadeOutDuration, Float fadeInDuration);

		void Change(Uint32 targetScene, Float fadeOutDuration, Float fadeInDuration);

		void Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene, Float coverDuration, Float revealDuration);

		void Change(Uint32 targetScene, Uint32 loadingScene, Float coverDuration, Float revealDuration);

		Float FadeAlpha()const;

		NODISCARD const SceneBlueprint* ConsumeSwitchedScene();

		Uint32 AssetID(const String& path)const;

		NODISCARD const String& CurrentName()const;

	private:
		enum class ChangeKind
		{
			None,

			Immediate,

			Loading,

			Fade,

			Cover,
		};

		struct ChangeRequest
		{
			ChangeKind kind_ = ChangeKind::None;

			std::filesystem::path targetScene_;

			std::filesystem::path loadingScene_;

			Float fadeOutDuration_ = 0.0f;

			Float fadeInDuration_ = 0.0f;

			Float coverDuration_ = 0.0f;

			Float revealDuration_ = 0.0f;
		};

	private:
		World* world_ = nullptr;

		ResourceCache* resource_ = nullptr;

		JobExecutor* executor_ = nullptr;

		SceneTransitionSystem transitionSystem_;

		ChangeRequest request_;

		const SceneBlueprint* switchedScene_ = nullptr;

		String currentName_;
	};
}
