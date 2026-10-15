#pragma once
#include <FoundationEngine/Prelude.h>

namespace SeedCore
{
	class SceneSystem;

	/**
	* [EN]
	* Gameplay-facing scene API: what game code uses to switch scenes
	* (with optional fade/loading-scene effects), read the current scene's
	* name and the fade overlay, and look up asset IDs by path. Forwards to
	* the SceneSystem the host owns, bound when that system is constructed
	* and on each of its Updates. A switch is only requested here and
	* happens at the end of the frame; while one is pending or running,
	* further requests are ignored. Loading/saving scenes and driving
	* transitions live on SceneSystem.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ゲームプレイ向けのシーンAPI。ゲームのコードがシーンを切り替え（任意で
	* フェード/ローディングシーンエフェクト付き）、現在のシーン名と
	* フェードオーバーレイを読み、パスからアセット ID を引くのに使う。
	* ホストが所有する SceneSystem へ転送する。転送先は、その SceneSystem の
	* 構築時と毎回の Update で束縛される。切り替えはここでは要求するだけで、
	* 実際にはフレームの終わりに行われる。要求が保留中か遷移が進行中の間は、
	* それ以降の要求は無視される。シーンの読み込み/保存と遷移の駆動は
	* SceneSystem にある。
	*/
	class SEEDCORE_API Scene
	{
	public:
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
		static void Change(const std::filesystem::path& targetScene);

		/**
		* [EN]
		* Overload of Change resolving targetScene from an asset ID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセット ID から targetScene を解決する Change のオーバーロード。
		*/
		static void Change(Uint32 targetScene);

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
		* targetScene がバックグラウンドで読み込まれている間は loadingScene
		* を表示する。別の切り替えが保留中であるか進行中であれば無視する。
		*/
		static void Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene);

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
		static void Change(Uint32 targetScene, Uint32 loadingScene);

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
		static void Change(const std::filesystem::path& targetScene, Float fadeOutDuration, Float fadeInDuration);

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
		static void Change(Uint32 targetScene, Float fadeOutDuration, Float fadeInDuration);

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
		static void Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene, Float coverDuration, Float revealDuration);

		/**
		* [EN]
		* Overload of the loading-scene cover/reveal Change resolving
		* both scenes from asset IDs.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセット ID から両方のシーンを解決する、ローディングシーン
		* 覆い隠し/表出版 Change のオーバーロード。
		*/
		static void Change(Uint32 targetScene, Uint32 loadingScene, Float coverDuration, Float revealDuration);

		/**
		* [EN]
		* Returns the current fade overlay alpha of the scene transition
		* (0 = fully visible scene, 1 = fully covered). 0 if no
		* SceneSystem is bound.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シーン遷移における、現在のフェードオーバーレイのアルファ値を返す
		* （0 = シーンが完全に見える状態、1 = 完全に覆われた状態）。
		* SceneSystem が束縛されていなければ 0。
		*/
		static Float FadeAlpha();

		/**
		* [EN]
		* Resolves path (project-root-relative, forward-slash) to its
		* AssetRecord ID via the bound SceneSystem's ResourceCache.
		* Returns 0 if no SceneSystem is bound or path is unknown.
		* This is the sanctioned entry point for gameplay code (SeedScript
		* subclasses) to dynamically look up an AssetRecord by path at runtime;
		* Tools/Python/RuntimePackager.py statically scans source for
		* calls to this function to determine which Assets must be
		* included in a packaged build, so avoid calling it with anything
		* other than a string literal.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* path（プロジェクトルート相対、フォワードスラッシュ）を、束縛済みの
		* SceneSystem の ResourceCache 経由で AssetRecord ID に解決する。
		* SceneSystem が束縛されていないか path が不明な場合は 0 を返す。
		* ゲームプレイコード（SeedScript のサブクラス）が実行時にパスから
		* AssetRecord を動的に引くための正規の入口である。
		* Tools/Python/RuntimePackager.py がこの関数への呼び出しをソース
		* コードから静的にスキャンし、パッケージビルドに含めるべき AssetRecord を
		* 判定するため、文字列リテラル以外を渡すのは避けること。
		*/
		static Uint32 AssetID(const String& path);

		/**
		* [EN]
		* Returns the name (file name without extension, e.g. "Title" for
		* "Title.scene") of the scene most recently switched to. A switch
		* requested this frame is not reflected until it happens, and
		* loading scenes shown during a transition do not count. Empty if
		* no scene has been loaded yet or no SceneSystem is bound.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直近に切り替わったシーンの名前（拡張子を除いたファイル名。
		* "Title.scene" なら "Title"）を返す。このフレームに要求した切り替えは、
		* 実際に起きるまで反映されない。遷移中に表示されるローディング
		* シーンは数えない。まだシーンが読み込まれていないか、SceneSystem が
		* 束縛されていなければ空。
		*/
		[[nodiscard]] static const String& CurrentName();

	private:
		/// [EN] SceneSystem binds itself as system_ when constructed and on every Update, and unbinds when destroyed.
		/// [JP] SceneSystem は構築時と毎回の Update で自身を system_ として束縛し、破棄時に外す。
		friend class SceneSystem;

	private:
		/// [EN] SceneSystem every call forwards to; owned by the host, null while none is bound.
		/// [JP] 全ての呼び出しの転送先となる SceneSystem。ホストが所有し、束縛されていない間は null。
		static SceneSystem* system_;
	};
}
