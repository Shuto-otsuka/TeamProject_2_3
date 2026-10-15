#include <FoundationEngine/Resource/Scene/SceneSystem.h>
#include <FoundationEngine/Resource/Scene/Scene.h>
#include <FoundationEngine/Resource/Scene/ScenePool.h>
#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
	/**
	* [EN]
	* Binds the world/cache/executor this system loads scenes into and
	* runs transitions with, and binds this system as the one Scene
	* forwards to.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シーンの読み込み先と遷移の実行に使う world/cache/executor を束縛し、
	* このシステムを Scene の転送先として束縛する。
	*/
	SceneSystem::SceneSystem(World& world, ResourceCache& cache, JobExecutor& executor) :world_(world), resource_(cache), executor_(executor)
	{
		/// [EN] Bind right away so scripts running before the first Update (e.g. on the first Play frame) can already reach this system.
		/// [JP] 最初の Update より前に動くスクリプト（例: Play の最初のフレーム）からも届くよう、すぐに束縛する。
		Scene::system_ = this;
	}

	/**
	* [EN]
	* Waits out any background load still running, and unbinds this
	* system from Scene if it is the bound one.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 実行中のバックグラウンド読み込みを待ち切り、このシステムが Scene に
	* 束縛されていれば束縛を外す。
	*/
	SceneSystem::~SceneSystem()
	{
		transitionSystem_.Reset();

		if (Scene::system_ == this)
		{
			Scene::system_ = nullptr;
		}
	}

	/**
	* [EN]
	* Captures the world's current state into a scratch SceneBlueprint and
	* writes it to path, together with how the caller wants it rendered.
	* On success the saved file's name becomes the current scene's name.
	* Returns whether saving succeeded.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world の現在の状態を作業用 SceneBlueprint へ取得し、呼び出し側が
	* 指定する描き方と共に path へ書き込む。成功すると、保存したファイルの
	* 名前が現在のシーン名になる。保存に成功したかどうかを返す。
	*/
	Bool SceneSystem::Save(const std::filesystem::path& path, const SceneVisual& visual)
	{
		/// [EN] Use the pool's shared scratch instance instead of allocating a new blueprint, since this is a one-shot operation with no need for pooled/deduplicated storage.
		/// [JP] 新しい設計図を確保する代わりに、プールの共有作業用インスタンスを使用する。これは使い捨ての操作であり、プール化/重複排除されたストレージは不要なため。
		SceneBlueprint& scene = resource_.GetScenePool().GetScratch();
		scene.Capture(world_);
		scene.visual_ = visual;

		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}

		if (!scene.Write(path))
		{
			return false;
		}

		/// [EN] The world now matches the saved file, so the scene is known by that file's name (e.g. after "save as").
		/// [JP] world は保存したファイルと一致したので、シーンはそのファイルの名前で呼ばれる（例: 名前を付けて保存した後）。
		currentName_ = String(path.stem().string());

		return true;
	}

	/**
	* [EN]
	* Immediately reads a scene from path into a scratch SceneBlueprint,
	* destroys every actor currently in the world, and instantiates the
	* loaded scene, bypassing the request queue. path is first looked up
	* as an asset name (a bare filename like "Foo.scene" works - see
	* ResourceCache::GetAssetID) and, if found, that asset's real
	* filesystem path is used instead; only if no matching asset exists
	* is path opened literally (relative to the current working
	* directory). When outVisual is given, how the scene wants to be
	* rendered is written there. Returns whether loading succeeded.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 要求の受け付けを通さず、path からシーンを作業用 SceneBlueprint へ
	* 即座に読み込み、world 内の現在の全 actor を破棄した上で、読み込んだ
	* シーンをインスタンス化する。path はまずアセット名として検索し
	* ("Foo.scene" のような単なるファイル名でもよい - ResourceCache::
	* GetAssetID 参照)、見つかればそのアセットの実際のファイルシステム
	* パスを代わりに使う。一致するアセットが無い場合のみ、path を文字通り
	* (カレントディレクトリからの相対で)開く。outVisual を渡した場合、
	* その Scene をどう描くかをそこへ書き込む。読み込みに成功したか
	* どうかを返す。
	*/
	Bool SceneSystem::Load(const std::filesystem::path& path, SceneVisual* outVisual)
	{
		std::filesystem::path resolvedPath = path;
		Uint32 assetID = resource_.GetAssetID(String(path.string()));
		if (assetID != 0)
		{
			AssetRecord* asset = resource_.GetAsset(assetID);
			if (asset)
			{
				resolvedPath = std::filesystem::path(asset->fullpath_.c_str());
			}
		}

		SceneBlueprint& scene = resource_.GetScenePool().GetScratch();
		if (!scene.Read(resolvedPath))
		{
			return false;
		}

		world_.DestroyActors();

		scene.Instantiate(world_, resource_);
		currentName_ = scene.Name();

		if (outVisual)
		{
			*outVisual = scene.Visual();
		}

		return true;
	}

	/**
	* [EN]
	* Overload of Load resolving the scene's path from an asset ID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID からシーンのパスを解決する Load のオーバーロード。
	*/
	Bool SceneSystem::Load(Uint32 assetID, SceneVisual* outVisual)
	{
		AssetRecord* asset = resource_.GetAsset(assetID);
		if (!asset)
		{
			return false;
		}

		return Load(std::filesystem::path(asset->fullpath_.c_str()), outVisual);
	}

	/**
	* [EN]
	* Captures the world's current state into blueprint, naming it after
	* the current scene.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* world の現在の状態を blueprint へ取得し、現在のシーン名を付ける。
	*/
	void SceneSystem::Capture(SceneBlueprint& blueprint)
	{
		blueprint.Capture(world_);
		blueprint.name_ = currentName_;
	}

	/**
	* [EN]
	* Instantiates blueprint into the world and makes its name the current
	* scene's name. Does not destroy the actors already there.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* blueprint を world へインスタンス化し、その名前を現在のシーン名に
	* する。既にいる actor は破棄しない。
	*/
	void SceneSystem::Restore(const SceneBlueprint& blueprint)
	{
		blueprint.Instantiate(world_, resource_);
		currentName_ = blueprint.Name();
	}

	/**
	* [EN]
	* Binds this system as the one Scene forwards to, carries out the
	* pending change request if any, then advances the transition state
	* machine by deltaTime.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このシステムを Scene の転送先として束縛し、保留中の切り替え要求が
	* あれば実行した上で、遷移の状態機械を deltaTime だけ進める。
	*/
	void SceneSystem::Update(Float deltaTime)
	{
		Scene::system_ = this;

		/// [EN] Scripts have finished for this frame, so the world can be swapped out safely here.
		/// [JP] このフレームのスクリプトは終わっているので、ここなら world を安全に入れ替えられる。
		switch (request_.kind_)
		{
		case ChangeKind::Immediate:
			transitionSystem_.LoadScene(world_, resource_, request_.targetScene_);
			break;
		case ChangeKind::Loading:
			transitionSystem_.LoadScene(world_, resource_, executor_, request_.targetScene_, request_.loadingScene_);
			break;
		case ChangeKind::Fade:
			transitionSystem_.LoadScene(world_, resource_, executor_, request_.targetScene_, request_.fadeOutDuration_, request_.fadeInDuration_);
			break;
		case ChangeKind::Cover:
			transitionSystem_.LoadScene(world_, resource_, executor_, request_.targetScene_, request_.loadingScene_, request_.coverDuration_, request_.revealDuration_);
			break;
		default:
			break;
		}
		request_ = ChangeRequest();

		transitionSystem_.Update(world_, resource_, deltaTime);

		/// [EN] Picked up here rather than by the host directly, so the current name follows every switch whether or not the host consumes it.
		/// [JP] ホストが直接受け取るのではなくここで拾う。ホストが受け取るかどうかに関わらず、現在のシーン名があらゆる切り替えに追従するようにするため。
		if (const SceneBlueprint* switched = transitionSystem_.ConsumeSwitchedScene())
		{
			currentName_ = switched->Name();
			switchedScene_ = switched;
		}
	}

	/**
	* [EN]
	* Drops any pending change request and aborts any in-progress
	* transition, returning it to Idle (see SceneTransitionSystem::Reset).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 保留中の切り替え要求を捨て、進行中の遷移を中断して Idle へ戻す
	* （SceneTransitionSystem::Reset を参照）。
	*/
	void SceneSystem::Reset()
	{
		request_ = ChangeRequest();
		switchedScene_ = nullptr;
		transitionSystem_.Reset();
	}

	/**
	* [EN]
	* Requests a synchronous switch to targetScene at the next Update.
	* Ignored while another request is pending or a transition is in
	* progress.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 次の Update での、targetScene への同期的な切り替えを要求する。
	* 別の要求が保留中であるか、遷移が進行中であれば無視する。
	*/
	void SceneSystem::Change(const std::filesystem::path& targetScene)
	{
		if (request_.kind_ != ChangeKind::None || transitionSystem_.Transitioning())
		{
			return;
		}

		request_.kind_ = ChangeKind::Immediate;
		request_.targetScene_ = targetScene;
	}

	/**
	* [EN]
	* Overload of Change resolving targetScene from an asset ID. Ignored
	* if the asset is unknown.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID から targetScene を解決する Change のオーバーロード。
	* アセットが不明であれば無視する。
	*/
	void SceneSystem::Change(Uint32 targetScene)
	{
		AssetRecord* targetAsset = resource_.GetAsset(targetScene);
		if (!targetAsset)
		{
			return;
		}

		Change(std::filesystem::path(targetAsset->fullpath_.c_str()));
	}

	/**
	* [EN]
	* Requests an asynchronous switch to targetScene at the next Update,
	* showing loadingScene while targetScene loads in the background.
	* Ignored while another request is pending or a transition is in
	* progress.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 次の Update での、targetScene への非同期切り替えを要求する。
	* targetScene がバックグラウンドで読み込まれている間は loadingScene を
	* 表示する。別の要求が保留中であるか、遷移が進行中であれば無視する。
	*/
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

	/**
	* [EN]
	* Overload of the loading-scene Change resolving both scenes from
	* asset IDs. Ignored if either asset is unknown.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID から両方のシーンを解決する、ローディングシーン版
	* Change のオーバーロード。どちらかのアセットが不明であれば無視する。
	*/
	void SceneSystem::Change(Uint32 targetScene, Uint32 loadingScene)
	{
		AssetRecord* targetAsset = resource_.GetAsset(targetScene);
		AssetRecord* loadingAsset = resource_.GetAsset(loadingScene);
		if (!targetAsset || !loadingAsset)
		{
			return;
		}

		Change(std::filesystem::path(targetAsset->fullpath_.c_str()), std::filesystem::path(loadingAsset->fullpath_.c_str()));
	}

	/**
	* [EN]
	* Requests an asynchronous switch to targetScene at the next Update,
	* using a fade-out/fade-in effect with the given durations. Ignored
	* while another request is pending or a transition is in progress.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 次の Update での、指定された長さのフェードアウト/フェードイン
	* エフェクトを使った targetScene への非同期切り替えを要求する。別の
	* 要求が保留中であるか、遷移が進行中であれば無視する。
	*/
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

	/**
	* [EN]
	* Overload of the fade-effect Change resolving targetScene from an
	* asset ID. Ignored if the asset is unknown.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID から targetScene を解決する、フェードエフェクト版
	* Change のオーバーロード。アセットが不明であれば無視する。
	*/
	void SceneSystem::Change(Uint32 targetScene, Float fadeOutDuration, Float fadeInDuration)
	{
		AssetRecord* targetAsset = resource_.GetAsset(targetScene);
		if (!targetAsset)
		{
			return;
		}

		Change(std::filesystem::path(targetAsset->fullpath_.c_str()), fadeOutDuration, fadeInDuration);
	}

	/**
	* [EN]
	* Requests an asynchronous switch to targetScene at the next Update,
	* using a loading-scene cover/reveal effect with the given durations.
	* Ignored while another request is pending or a transition is in
	* progress.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 次の Update での、指定された長さのローディングシーンによる
	* 覆い隠し/表出エフェクトを使った targetScene への非同期切り替えを
	* 要求する。別の要求が保留中であるか、遷移が進行中であれば無視する。
	*/
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

	/**
	* [EN]
	* Overload of the loading-scene cover/reveal Change resolving both
	* scenes from asset IDs. Ignored if either asset is unknown.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID から両方のシーンを解決する、ローディングシーン
	* 覆い隠し/表出版 Change のオーバーロード。どちらかのアセットが
	* 不明であれば無視する。
	*/
	void SceneSystem::Change(Uint32 targetScene, Uint32 loadingScene, Float coverDuration, Float revealDuration)
	{
		AssetRecord* targetAsset = resource_.GetAsset(targetScene);
		AssetRecord* loadingAsset = resource_.GetAsset(loadingScene);
		if (!targetAsset || !loadingAsset)
		{
			return;
		}

		Change(std::filesystem::path(targetAsset->fullpath_.c_str()), std::filesystem::path(loadingAsset->fullpath_.c_str()), coverDuration, revealDuration);
	}

	/**
	* [EN]
	* Returns the current fade overlay alpha of the scene transition.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シーン遷移における、現在のフェードオーバーレイのアルファ値を返す。
	*/
	Float SceneSystem::FadeAlpha()const
	{
		return transitionSystem_.GetFadeAlpha();
	}

	/**
	* [EN]
	* Returns the scene a transition has switched to since the last call,
	* or nullptr if none, clearing that state in the same step.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 前回の呼び出し以降に遷移が切り替えた先のシーンを返す。無ければ
	* nullptr。同時にその状態を取り下げる。
	*/
	const SceneBlueprint* SceneSystem::ConsumeSwitchedScene()
	{
		const SceneBlueprint* switched = switchedScene_;
		switchedScene_ = nullptr;
		return switched;
	}

	/**
	* [EN]
	* Resolves path (project-root-relative, forward-slash) to its
	* AssetRecord ID via the bound ResourceCache. Returns 0 if path is
	* unknown.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* path（プロジェクトルート相対、フォワードスラッシュ）を、束縛済みの
	* ResourceCache 経由で AssetRecord ID に解決する。path が不明な場合は
	* 0 を返す。
	*/
	Uint32 SceneSystem::AssetID(const String& path)const
	{
		return resource_.GetAssetID(path);
	}

	/**
	* [EN]
	* Returns the name of the scene most recently switched to, loaded or
	* restored. Empty if no scene has been loaded yet.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直近に切り替え・読み込み・復元されたシーンの名前を返す。まだシーンが
	* 読み込まれていなければ空。
	*/
	const String& SceneSystem::CurrentName()const
	{
		return currentName_;
	}
}
