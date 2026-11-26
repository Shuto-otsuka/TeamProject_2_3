#include <FoundationEngine/Resource/Scene/SceneSystem.h>
#include <FoundationEngine/Resource/Scene/Scene.h>
#include <FoundationEngine/Resource/Scene/ScenePool.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/Resource/Config/GameConfig.h>
#include <FoundationEngine/Resource/Config/EditorConfig.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	Bool SceneSystem::Initialize(World& world, ResourceCache& cache, JobExecutor& executor, const GameConfig& gameConfig)
	{
		world_ = &world;
		resource_ = &cache;
		executor_ = &executor;

		Scene::system_ = this;

		if (gameConfig.initialScenePath_.view().empty())
		{
			SC_LOG_ERROR("起動シーンが設定されていません。GameConfig の初回シーンを設定してください");
		}
		else if (!Load(std::filesystem::path(gameConfig.initialScenePath_.str())))
		{
			SC_LOG_ERROR("起動シーンの読み込みに失敗しました: {}", gameConfig.initialScenePath_.str());
		}
		return true;
	}

	Bool SceneSystem::Initialize(World& world, ResourceCache& cache, JobExecutor& executor, const EditorConfig& editorConfig)
	{
		world_ = &world;
		resource_ = &cache;
		executor_ = &executor;

		Scene::system_ = this;

		if (!editorConfig.lastScenePath_.view().empty())
		{
			Load(std::filesystem::path(editorConfig.lastScenePath_.str()));
		}
		return true;
	}

	SceneSystem::~SceneSystem()
	{
		transitionSystem_.Reset();

		if (Scene::system_ == this)
		{
			Scene::system_ = nullptr;
		}
	}

	Bool SceneSystem::Save(const std::filesystem::path& path)
	{
		SceneBlueprint& scene = resource_->Scenes().GetScratch();
		scene.Capture(*world_);

		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}

		if (!scene.Write(path))
		{
			return false;
		}

		currentName_ = String(path.stem().string());

		return true;
	}

	Bool SceneSystem::Load(const std::filesystem::path& path)
	{
		std::filesystem::path resolvedPath = path;
		Uint32 assetID = resource_->Find(String(path.string()));
		if (assetID != 0)
		{
			AssetRecord* asset = resource_->Record(assetID);
			if (asset)
			{
				resolvedPath = std::filesystem::path(asset->fullpath_.c_str());
			}
		}

		SceneBlueprint& scene = resource_->Scenes().GetScratch();
		if (!scene.Read(resolvedPath))
		{
			return false;
		}

		WorldAccessor::Clear(*world_);

		scene.Instantiate(*world_, *resource_);
		currentName_ = scene.Name();

		return true;
	}

	Bool SceneSystem::Load(Uint32 assetID)
	{
		AssetRecord* asset = resource_->Record(assetID);
		if (!asset)
		{
			return false;
		}

		return Load(std::filesystem::path(asset->fullpath_.c_str()));
	}

	void SceneSystem::Capture(SceneBlueprint& blueprint)
	{
		blueprint.Capture(*world_);
		blueprint.name_ = currentName_;
	}

	void SceneSystem::Restore(const SceneBlueprint& blueprint)
	{
		blueprint.Instantiate(*world_, *resource_);
		currentName_ = blueprint.Name();
	}

	void SceneSystem::Update(Float deltaTime)
	{
		Scene::system_ = this;

		switch (request_.kind_)
		{
		case ChangeKind::Immediate:
			transitionSystem_.LoadScene(*world_, *resource_, request_.targetScene_);
			break;
		case ChangeKind::Loading:
			transitionSystem_.LoadScene(*world_, *resource_, *executor_, request_.targetScene_, request_.loadingScene_);
			break;
		case ChangeKind::Fade:
			transitionSystem_.LoadScene(*world_, *resource_, *executor_, request_.targetScene_, request_.fadeOutDuration_, request_.fadeInDuration_);
			break;
		case ChangeKind::Cover:
			transitionSystem_.LoadScene(*world_, *resource_, *executor_, request_.targetScene_, request_.loadingScene_, request_.coverDuration_, request_.revealDuration_);
			break;
		default:
			break;
		}
		request_ = ChangeRequest();

		transitionSystem_.Update(*world_, *resource_, deltaTime);

		if (const SceneBlueprint* switched = transitionSystem_.ConsumeSwitchedScene())
		{
			currentName_ = switched->Name();
			switchedScene_ = switched;
		}
	}

	void SceneSystem::Reset()
	{
		request_ = ChangeRequest();
		switchedScene_ = nullptr;
		transitionSystem_.Reset();
	}

	void SceneSystem::Change(const std::filesystem::path& targetScene)
	{
		if (request_.kind_ != ChangeKind::None || transitionSystem_.Transitioning())
		{
			return;
		}

		request_.kind_ = ChangeKind::Immediate;
		request_.targetScene_ = targetScene;
	}

	void SceneSystem::Change(Uint32 targetScene)
	{
		AssetRecord* targetAsset = resource_->Record(targetScene);
		if (!targetAsset)
		{
			return;
		}

		Change(std::filesystem::path(targetAsset->fullpath_.c_str()));
	}

	void SceneSystem::Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene)
	{
		if (request_.kind_ != ChangeKind::None || transitionSystem_.Transitioning())
		{
			return;
		}

		request_.kind_ = ChangeKind::Loading;
		request_.targetScene_ = targetScene;
		request_.loadingScene_ = loadingScene;
	}

	void SceneSystem::Change(Uint32 targetScene, Uint32 loadingScene)
	{
		AssetRecord* targetAsset = resource_->Record(targetScene);
		AssetRecord* loadingAsset = resource_->Record(loadingScene);
		if (!targetAsset || !loadingAsset)
		{
			return;
		}

		Change(std::filesystem::path(targetAsset->fullpath_.c_str()), std::filesystem::path(loadingAsset->fullpath_.c_str()));
	}

	void SceneSystem::Change(const std::filesystem::path& targetScene, Float fadeOutDuration, Float fadeInDuration)
	{
		if (request_.kind_ != ChangeKind::None || transitionSystem_.Transitioning())
		{
			return;
		}

		request_.kind_ = ChangeKind::Fade;
		request_.targetScene_ = targetScene;
		request_.fadeOutDuration_ = fadeOutDuration;
		request_.fadeInDuration_ = fadeInDuration;
	}

	void SceneSystem::Change(Uint32 targetScene, Float fadeOutDuration, Float fadeInDuration)
	{
		AssetRecord* targetAsset = resource_->Record(targetScene);
		if (!targetAsset)
		{
			return;
		}

		Change(std::filesystem::path(targetAsset->fullpath_.c_str()), fadeOutDuration, fadeInDuration);
	}

	void SceneSystem::Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene, Float coverDuration, Float revealDuration)
	{
		if (request_.kind_ != ChangeKind::None || transitionSystem_.Transitioning())
		{
			return;
		}

		request_.kind_ = ChangeKind::Cover;
		request_.targetScene_ = targetScene;
		request_.loadingScene_ = loadingScene;
		request_.coverDuration_ = coverDuration;
		request_.revealDuration_ = revealDuration;
	}

	void SceneSystem::Change(Uint32 targetScene, Uint32 loadingScene, Float coverDuration, Float revealDuration)
	{
		AssetRecord* targetAsset = resource_->Record(targetScene);
		AssetRecord* loadingAsset = resource_->Record(loadingScene);
		if (!targetAsset || !loadingAsset)
		{
			return;
		}

		Change(std::filesystem::path(targetAsset->fullpath_.c_str()), std::filesystem::path(loadingAsset->fullpath_.c_str()), coverDuration, revealDuration);
	}

	Float SceneSystem::FadeAlpha()const
	{
		return transitionSystem_.GetFadeAlpha();
	}

	const SceneBlueprint* SceneSystem::ConsumeSwitchedScene()
	{
		const SceneBlueprint* switched = switchedScene_;
		switchedScene_ = nullptr;
		return switched;
	}

	Uint32 SceneSystem::AssetID(const String& path)const
	{
		return resource_->Find(path);
	}

	const String& SceneSystem::CurrentName()const
	{
		return currentName_;
	}
}
