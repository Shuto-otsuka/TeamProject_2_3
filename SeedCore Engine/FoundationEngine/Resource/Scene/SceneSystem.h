#pragma once
#include <FoundationEngine/Resource/Scene/SceneBlueprint.h>
#include <FoundationEngine/Resource/Scene/SceneTransitionSystem.h>
#include <FoundationEngine/Utility/NonTransferable.h>

namespace SeedCore
{
	class World;
	class ResourceCache;
	class JobExecutor;

	/**
	* [EN]
	* Host-side scene API, owned by the host that runs the main loop
	* (not by World, since it replaces the World's contents from
	* outside). Loads/saves scenes for the editor, and drives the scene
	* transition state machine every frame. Scene switches are taken as
	* requests and carried out at the start of the next Update, so a
	* script asking for a switch mid-update never has the world emptied
	* under it; the first request wins and later ones are ignored while
	* it is pending or its transition is running. Gameplay code reaches
	* this through Scene, which forwards to the SceneSystem bound on
	* construction and on every Update.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ホスト向けのシーンAPI。メインループを回すホストが所有する（World の
	* 中身を外から入れ替えるものなので World には持たせない）。エディタ
	* 向けにシーンを読み込み/保存し、シーン遷移の状態機械を毎フレーム
	* 駆動する。シーン切り替えは要求として受け取り、次の Update の頭で
	* 実行するため、更新の途中で切り替えを求めたスクリプトの足元で world
	* が空になることは無い。先に来た要求が勝ち、それが保留中か遷移が
	* 進行中の間に来た要求は無視する。ゲームプレイコードは Scene 経由で
	* ここに届く。Scene は、構築時と毎回の Update で束縛された
	* SceneSystem へ転送する。
	*/
	class SEEDCORE_API SceneSystem :public NonTransferable
	{
	public:
		/**
		* [EN]
		* Binds the world/cache/executor this system loads scenes into and
		* runs transitions with, and binds this system as the one Scene
		* forwards to.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シーンの読み込み先と遷移の実行に使う world/cache/executor を
		* 束縛し、このシステムを Scene の転送先として束縛する。
		*/
		SceneSystem(World& world, ResourceCache& cache, JobExecutor& executor);

		/**
		* [EN]
		* Waits out any background load still running, and unbinds this
		* system from Scene if it is the bound one. The JobExecutor given
		* on construction must still be alive.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 実行中のバックグラウンド読み込みを待ち切り、このシステムが Scene に
		* 束縛されていれば束縛を外す。構築時に渡した JobExecutor はまだ
		* 生きている必要がある。
		*/
		~SceneSystem();

	public:
		/**
		* [EN]
		* Captures the world's current state into a scratch SceneBlueprint
		* and writes it to path, together with how the caller wants it
		* rendered. Leaving visual out saves a scene that carries nothing
		* of its own. On success the saved file's name becomes the current
		* scene's name. Returns whether saving succeeded.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* world の現在の状態を作業用 SceneBlueprint へ取得し、呼び出し側が
		* 指定する描き方と共に path へ書き込む。visual を省いた場合、その
		* Scene は描き方を持たないものとして保存される。成功すると、保存した
		* ファイルの名前が現在のシーン名になる。保存に成功したかどうかを
		* 返す。
		*/
		Bool Save(const std::filesystem::path& path, const SceneVisual& visual = SceneVisual());

		/**
		* [EN]
		* Immediately reads a scene from path into a scratch
		* SceneBlueprint, destroys every actor currently in the world, and
		* instantiates the loaded scene, bypassing the request queue. When
		* outVisual is given, how the scene wants to be rendered is written
		* there; parts a scene never carried come back empty. Returns
		* whether loading succeeded.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 要求の受け付けを通さず、path からシーンを作業用 SceneBlueprint へ
		* 即座に読み込み、world 内の現在の全 actor を破棄した上で、読み込んだ
		* シーンをインスタンス化する。outVisual を渡した場合、その Scene を
		* どう描くかをそこへ書き込む。その Scene が元から持っていない項目は
		* 空で返る。読み込みに成功したかどうかを返す。
		*/
		Bool Load(const std::filesystem::path& path, SceneVisual* outVisual = nullptr);

		/**
		* [EN]
		* Overload of Load resolving the scene's path from an asset ID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセット ID からシーンのパスを解決する Load のオーバーロード。
		*/
		Bool Load(Uint32 assetID, SceneVisual* outVisual = nullptr);

		/**
		* [EN]
		* Captures the world's current state into blueprint, naming it
		* after the current scene, so Restore() can bring both back later
		* (e.g. the edited scene around an editor Play session).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* world の現在の状態を blueprint へ取得し、現在のシーン名を付ける。
		* 後で Restore() により両方を戻せるようにするため（例: エディタの
		* Play の前後で編集中のシーンを保つとき）。
		*/
		void Capture(SceneBlueprint& blueprint);

		/**
		* [EN]
		* Instantiates blueprint into the world and makes its name the
		* current scene's name. Does not destroy the actors already there.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* blueprint を world へインスタンス化し、その名前を現在のシーン名に
		* する。既にいる actor は破棄しない。
		*/
		void Restore(const SceneBlueprint& blueprint);

	public:
		/**
		* [EN]
		* Binds this system as the one Scene forwards to, carries out the
		* pending change request if any, then advances the transition
		* state machine by deltaTime. Call once per frame after the
		* scripts have run.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このシステムを Scene の転送先として束縛し、保留中の切り替え要求が
		* あれば実行した上で、遷移の状態機械を deltaTime だけ進める。
		* スクリプトの実行後に毎フレーム1回呼ぶ。
		*/
		void Update(Float deltaTime);

		/**
		* [EN]
		* Drops any pending change request and aborts any in-progress
		* transition, returning it to Idle (see SceneTransitionSystem::Reset).
		* Call when leaving editor Play mode so a switch asked for during
		* Play does not happen afterward against stale actors.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 保留中の切り替え要求を捨て、進行中の遷移を中断して Idle へ戻す
		* （SceneTransitionSystem::Reset を参照）。エディタの Play モードを
		* 抜ける際に呼び、Play 中に求められた切り替えがその後に古くなった
		* actor に対して起きないようにする。
		*/
		void Reset();

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
		void Change(const std::filesystem::path& targetScene);

		/**
		* [EN]
		* Overload of Change resolving targetScene from an asset ID.
		* Ignored if the asset is unknown.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセット ID から targetScene を解決する Change のオーバーロード。
		* アセットが不明であれば無視する。
		*/
		void Change(Uint32 targetScene);

		/**
		* [EN]
		* Requests an asynchronous switch to targetScene at the next
		* Update, showing loadingScene while targetScene loads in the
		* background. Ignored while another request is pending or a
		* transition is in progress.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 次の Update での、targetScene への非同期切り替えを要求する。
		* targetScene がバックグラウンドで読み込まれている間は loadingScene
		* を表示する。別の要求が保留中であるか、遷移が進行中であれば無視する。
		*/
		void Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene);

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
		void Change(Uint32 targetScene, Uint32 loadingScene);

		/**
		* [EN]
		* Requests an asynchronous switch to targetScene at the next
		* Update, using a fade-out/fade-in effect with the given
		* durations. Ignored while another request is pending or a
		* transition is in progress.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 次の Update での、指定された長さのフェードアウト/フェードイン
		* エフェクトを使った targetScene への非同期切り替えを要求する。
		* 別の要求が保留中であるか、遷移が進行中であれば無視する。
		*/
		void Change(const std::filesystem::path& targetScene, Float fadeOutDuration, Float fadeInDuration);

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
		void Change(Uint32 targetScene, Float fadeOutDuration, Float fadeInDuration);

		/**
		* [EN]
		* Requests an asynchronous switch to targetScene at the next
		* Update, using a loading-scene cover/reveal effect with the given
		* durations. Ignored while another request is pending or a
		* transition is in progress.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 次の Update での、指定された長さのローディングシーンによる
		* 覆い隠し/表出エフェクトを使った targetScene への非同期切り替えを
		* 要求する。別の要求が保留中であるか、遷移が進行中であれば無視する。
		*/
		void Change(const std::filesystem::path& targetScene, const std::filesystem::path& loadingScene, Float coverDuration, Float revealDuration);

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
		void Change(Uint32 targetScene, Uint32 loadingScene, Float coverDuration, Float revealDuration);

		/**
		* [EN]
		* Returns the current fade overlay alpha of the scene transition.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シーン遷移における、現在のフェードオーバーレイのアルファ値を返す。
		*/
		Float FadeAlpha()const;

		/**
		* [EN]
		* Returns the scene a transition has switched to since the last
		* call, or nullptr if none, clearing that state in the same step.
		* Lets the host that owns the main loop pick up whatever it keeps
		* per scene - the scene's own contents are already instantiated
		* into the world. Loading scenes shown during a transition are never
		* returned. The returned scene stays valid until the next transition
		* begins.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前回の呼び出し以降に遷移が切り替えた先のシーンを返す。無ければ
		* nullptr。同時にその状態を取り下げる。メインループを所有する
		* ホストが、シーンごとに持つものを受け取るために使う - シーン自体の
		* 中身は既に world へインスタンス化済み。遷移中に表示される
		* ローディングシーンは返さない。返したシーンは次の遷移が始まるまで
		* 有効。
		*/
		NODISCARD const SceneBlueprint* ConsumeSwitchedScene();

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
		Uint32 AssetID(const String& path)const;

		/**
		* [EN]
		* Returns the name (file name without extension, e.g. "Title" for
		* "Title.scene") of the scene most recently switched to, loaded or
		* restored. Loading scenes shown during a transition do not count,
		* so the previous scene's name stays until the target arrives.
		* Empty if no scene has been loaded yet.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直近に切り替え・読み込み・復元されたシーンの名前（拡張子を除いた
		* ファイル名。"Title.scene" なら "Title"）を返す。遷移中に表示される
		* ローディングシーンは数えないため、ターゲットが現れるまでは前の
		* シーンの名前のまま。まだシーンが読み込まれていなければ空。
		*/
		NODISCARD const String& CurrentName()const;

	private:
		/**
		* [EN]
		* Which kind of switch a ChangeRequest asks for.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ChangeRequest が求める切り替えの種類。
		*/
		enum class ChangeKind
		{
			/// [EN] No request is pending.
			/// [JP] 保留中の要求は無い。
			None,

			/// [EN] Synchronous switch.
			/// [JP] 同期的な切り替え。
			Immediate,

			/// [EN] Asynchronous switch showing a loading scene in place of the current one.
			/// [JP] 現在のシーンの代わりにローディングシーンを表示する非同期切り替え。
			Loading,

			/// [EN] Asynchronous switch with a fade-out/fade-in effect.
			/// [JP] フェードアウト/フェードインエフェクト付きの非同期切り替え。
			Fade,

			/// [EN] Asynchronous switch with a loading-scene cover/reveal effect.
			/// [JP] ローディングシーンによる覆い隠し/表出エフェクト付きの非同期切り替え。
			Cover,
		};

		/**
		* [EN]
		* A scene switch asked for through Change, held until the next Update.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Change で求められたシーン切り替え。次の Update まで保持する。
		*/
		struct ChangeRequest
		{
			/// [EN] Kind of switch asked for; None when nothing is pending.
			/// [JP] 求められた切り替えの種類。何も保留されていなければ None。
			ChangeKind kind_ = ChangeKind::None;

			/// [EN] Scene to switch to.
			/// [JP] 切り替え先のシーン。
			std::filesystem::path targetScene_;

			/// [EN] Scene shown while targetScene_ loads (Loading and Cover only).
			/// [JP] targetScene_ の読み込み中に表示するシーン（Loading と Cover のみ）。
			std::filesystem::path loadingScene_;

			/// [EN] Fade-out duration in seconds (Fade only).
			/// [JP] フェードアウトの長さ（秒、Fade のみ）。
			Float fadeOutDuration_ = 0.0f;

			/// [EN] Fade-in duration in seconds (Fade only).
			/// [JP] フェードインの長さ（秒、Fade のみ）。
			Float fadeInDuration_ = 0.0f;

			/// [EN] Cover duration in seconds (Cover only).
			/// [JP] 覆い隠しの長さ（秒、Cover のみ）。
			Float coverDuration_ = 0.0f;

			/// [EN] Reveal duration in seconds (Cover only).
			/// [JP] 表出の長さ（秒、Cover のみ）。
			Float revealDuration_ = 0.0f;
		};

	private:
		/// [EN] World scenes are loaded into and switched in.
		/// [JP] シーンの読み込み先、かつ切り替え先の World。
		World& world_;

		/// [EN] ResourceCache used to resolve scene names/IDs to files.
		/// [JP] シーン名/ID をファイルへ解決するのに使う ResourceCache。
		ResourceCache& resource_;

		/// [EN] JobExecutor that background-loaded transitions run on.
		/// [JP] バックグラウンド読み込みを伴う遷移を実行する JobExecutor。
		JobExecutor& executor_;

		/// [EN] Scene transition state machine carrying out the requests.
		/// [JP] 要求を実行するシーン遷移の状態機械。
		SceneTransitionSystem transitionSystem_;

		/// [EN] The switch asked for since the last Update, if any.
		/// [JP] 前回の Update 以降に求められた切り替え（あれば）。
		ChangeRequest request_;

		/// [EN] Scene a transition switched to and the host has not yet consumed (see ConsumeSwitchedScene).
		/// [JP] 遷移が切り替えた先で、ホストがまだ受け取っていないシーン（ConsumeSwitchedScene を参照）。
		const SceneBlueprint* switchedScene_ = nullptr;

		/// [EN] Name of the scene most recently switched to, loaded or restored (see CurrentName).
		/// [JP] 直近に切り替え・読み込み・復元されたシーンの名前（CurrentName を参照）。
		String currentName_;
	};
}
