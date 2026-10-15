#include <FoundationEngine/Resource/Scene/Scene.h>
#include <FoundationEngine/Resource/Scene/SceneSystem.h>

namespace SeedCore
{
	SceneSystem* Scene::system_ = nullptr;

	/**
	* [EN]
	* Requests a synchronous switch to targetScene at the end of the
	* frame. Ignored while another switch is pending or in progress.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームの終わりでの、targetScene への同期的な切り替えを要求する。
	* 別の切り替えが保留中であるか進行中であれば無視する。
	*/
	void Scene::Change(const std::filesystem::path& targetScene)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene);
	}

	/**
	* [EN]
	* Overload of Change resolving targetScene from an asset ID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID から targetScene を解決する Change のオーバーロード。
	*/
	void Scene::Change(Uint32 targetScene)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene);
	}

	/**
	* [EN]
	* Requests an asynchronous switch to targetScene at the end of the
	* frame, showing loadingScene while targetScene loads in the
	* background. Ignored while another switch is pending or in progress.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームの終わりでの、targetScene への非同期切り替えを要求する。
	* targetScene がバックグラウンドで読み込まれている間は loadingScene を
	* 表示する。別の切り替えが保留中であるか進行中であれば無視する。
	*/
	void Scene::Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene, loadingScene);
	}

	/**
	* [EN]
	* Overload of the loading-scene Change resolving both scenes from asset IDs.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID から両方のシーンを解決する、ローディングシーン版
	* Change のオーバーロード。
	*/
	void Scene::Change(Uint32 targetScene, Uint32 loadingScene)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene, loadingScene);
	}

	/**
	* [EN]
	* Requests an asynchronous switch to targetScene at the end of the
	* frame, using a fade-out/fade-in effect with the given durations.
	* Ignored while another switch is pending or in progress.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームの終わりでの、指定された長さのフェードアウト/フェードイン
	* エフェクトを使った targetScene への非同期切り替えを要求する。別の
	* 切り替えが保留中であるか進行中であれば無視する。
	*/
	void Scene::Change(const std::filesystem::path& targetScene, Float fadeOutDuration, Float fadeInDuration)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene, fadeOutDuration, fadeInDuration);
	}

	/**
	* [EN]
	* Overload of the fade-effect Change resolving targetScene from an asset ID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID から targetScene を解決する、フェードエフェクト版
	* Change のオーバーロード。
	*/
	void Scene::Change(Uint32 targetScene, Float fadeOutDuration, Float fadeInDuration)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene, fadeOutDuration, fadeInDuration);
	}

	/**
	* [EN]
	* Requests an asynchronous switch to targetScene at the end of the
	* frame, using a loading-scene cover/reveal effect with the given
	* durations. Ignored while another switch is pending or in progress.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* フレームの終わりでの、指定された長さのローディングシーンによる
	* 覆い隠し/表出エフェクトを使った targetScene への非同期切り替えを
	* 要求する。別の切り替えが保留中であるか進行中であれば無視する。
	*/
	void Scene::Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene, Float coverDuration, Float revealDuration)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene, loadingScene, coverDuration, revealDuration);
	}

	/**
	* [EN]
	* Overload of the loading-scene cover/reveal Change resolving both
	* scenes from asset IDs.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット ID から両方のシーンを解決する、ローディングシーン
	* 覆い隠し/表出版 Change のオーバーロード。
	*/
	void Scene::Change(Uint32 targetScene, Uint32 loadingScene, Float coverDuration, Float revealDuration)
	{
		if (!system_)
		{
			return;
		}

		system_->Change(targetScene, loadingScene, coverDuration, revealDuration);
	}

	/**
	* [EN]
	* Returns the current fade overlay alpha of the scene transition. 0 if
	* no SceneSystem is bound.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シーン遷移における、現在のフェードオーバーレイのアルファ値を返す。
	* SceneSystem が束縛されていなければ 0。
	*/
	Float Scene::FadeAlpha()
	{
		if (!system_)
		{
			return 0.0f;
		}

		return system_->FadeAlpha();
	}

	/**
	* [EN]
	* Resolves path (project-root-relative, forward-slash) to its
	* AssetRecord ID via the bound SceneSystem's ResourceCache. Returns 0
	* if no SceneSystem is bound or path is unknown. Scanned for by
	* Tools/Python/RuntimePackager.py, so pass a string literal.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* path（プロジェクトルート相対、フォワードスラッシュ）を、束縛済みの
	* SceneSystem の ResourceCache 経由で AssetRecord ID に解決する。
	* SceneSystem が束縛されていないか path が不明な場合は 0 を返す。
	* Tools/Python/RuntimePackager.py がスキャンするため、文字列リテラルを
	* 渡すこと。
	*/
	Uint32 Scene::AssetID(const String& path)
	{
		if (!system_)
		{
			return 0;
		}

		return system_->AssetID(path);
	}

	/**
	* [EN]
	* Returns the name of the scene most recently switched to. Empty if no
	* scene has been loaded yet or no SceneSystem is bound.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 直近に切り替わったシーンの名前を返す。まだシーンが読み込まれて
	* いないか、SceneSystem が束縛されていなければ空。
	*/
	const String& Scene::CurrentName()
	{
		/// [EN] Returned by reference while nothing is bound, so it has to outlive the call.
		/// [JP] 何も束縛されていない間に参照で返すため、呼び出しより長く生きている必要がある。
		static const String emptyName;

		if (!system_)
		{
			return emptyName;
		}

		return system_->CurrentName();
	}
}
