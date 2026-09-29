#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Utility/NonTransferable.h>

namespace SeedCore
{
	class World;
	class PluginHost;
	class PluginModule;
	class CsharpHost;

	/**
	* [EN]
	* Editor-only development loop for the UserProject scripts: watches its
	* source tree, auto-builds UserProject.Cplusplus.dll via MSBuild when a
	* .h/.cpp changes and UserProject.Csharp.dll via dotnet build when a .cs
	* changes, and — once a build finishes — asks the PluginHost to reload
	* the C++ plugin or the CsharpHost to reload the C# scripts. The
	* source-change detection debounces on a stable timestamp window before
	* acting, since an editor can touch a file's write time more than once
	* while saving it. A C# reload that becomes ready during Play waits
	* until Play stops.
	*
	* The module lifecycle itself (shadow-copy load, entry-point
	* resolution, reflection-registry bookkeeping, component
	* capture/restore across a reload) lives in PluginHost / PluginModule
	* and CsharpHost, and is shared with the game runtime; this class only
	* adds the editor-side "rebuild from source" half.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* UserProject のスクリプト用の、エディタ限定の開発ループ: そのソース
	* ツリーを監視し、.h/.cpp が変わると MSBuild で UserProject.Cplusplus.dll
	* を、.cs が変わると dotnet build で UserProject.Csharp.dll を自動
	* ビルドし、ビルド完了時に PluginHost へ C++ プラグインの、CsharpHost
	* へ C# スクリプトのリロードを依頼する。ソース変更検知は、タイム
	* スタンプが一定時間安定するまで待ってから動作する（エディタは保存中に
	* 最終更新時刻を複数回更新することがあるため）。再生中に準備が
	* できた C# のリロードは、再生が止まるまで待つ。
	*
	* モジュールのライフサイクル自体（シャドウコピーからのロード、
	* エントリポイント解決、リフレクションレジストリの管理、リロードを
	* またいだコンポーネントの取得/復元）は PluginHost / PluginModule と
	* CsharpHost にあり、ゲームランタイムと共有される; このクラスは
	* エディタ側の「ソースからの再ビルド」の半分だけを担う。
	*/
	class HotReload :public NonTransferable
	{
	public:
		HotReload() = default;
		~HotReload();

		/**
		* [EN]
		* Binds the PluginHost and CsharpHost this class drives and caches
		* the already-loaded UserProject plugin (nullptr if
		* UserProject.Cplusplus.dll was not among the loaded plugins). Call
		* once after PluginHost::Load and CsharpHost::Load.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このクラスが駆動する PluginHost と CsharpHost を束縛し、そこで
		* ロード済みの UserProject プラグインをキャッシュする
		* （UserProject.Cplusplus.dll がロード済みプラグインに無ければ
		* nullptr）。PluginHost::Load と CsharpHost::Load の後に一度呼ぶこと。
		*/
		void Initialize(PluginHost& pluginHost, CsharpHost& csharpHost);

		/**
		* [EN]
		* Polls the running build processes and the UserProject source tree
		* once per frame; triggers an auto-build when the source changed,
		* and reloads the scripts once a build this class started has
		* finished.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 実行中のビルドプロセスと UserProject のソースツリーを毎フレーム
		* 確認する; ソースが変わっていれば自動ビルドを開始し、このクラスが
		* 起動したビルドが完了したらスクリプトをリロードする。
		*/
		void Tick(World& world);

	private:
		struct BuildTask
		{
			/// [EN] Handle of the currently running background build process (nullptr when idle).
			/// [JP] 現在実行中のバックグラウンドビルドプロセスのハンドル（アイドル時は nullptr）。
			HANDLE process_ = nullptr;

			/// [EN] Read end of the pipe the build process's stdout/stderr are redirected to (nullptr when idle). Drained every Tick() so the child never blocks on a full pipe buffer.
			/// [JP] ビルドプロセスの標準出力/標準エラーのリダイレクト先パイプの読み取り側（アイドル時は nullptr）。子プロセスがパイプバッファ満杯でブロックしないよう、毎 Tick() で読み出す。
			HANDLE outputPipe_ = nullptr;

			/// [EN] Accumulated stdout/stderr text from the currently (or most recently) running build, for logging on failure.
			/// [JP] 現在（または直近）のビルドの標準出力/標準エラーの蓄積テキスト。失敗時のログ出力に使う。
			std::string output_;

			/// [EN] Set when a build this class started succeeds, telling Tick() to reload the scripts it built.
			/// [JP] このクラスが起動したビルドが成功したときに立つ。Tick() にそのビルドのスクリプトをリロードするよう伝える。
			Bool reloadRequested_ = false;

			/// [EN] Latest last-write time observed under the source tree, at the moment a build was last triggered.
			/// [JP] 直近にビルドをトリガーした時点で観測していた、ソースツリー内の最新の最終更新時刻。
			Uint64 lastTriggeredWriteTime_ = 0;

			/// [EN] Source-tree write time seen on the previous scan, awaiting confirmation that it has stopped changing.
			/// [JP] 前回スキャン時に観測したソースツリーの更新時刻。変化が止まったかどうかの確認待ち状態で使う。
			Uint64 pendingWriteTime_ = 0;

			/// [EN] GetTickCount64() timestamp when pendingWriteTime_ was first observed.
			/// [JP] pendingWriteTime_ を最初に観測した時点の GetTickCount64()。
			Uint64 pendingStableSinceTick_ = 0;

			/// [EN] GetTickCount64() timestamp of the last source-tree scan, used to throttle scans to a fixed interval.
			/// [JP] 直近にソースツリーをスキャンした時点の GetTickCount64()。スキャン頻度を一定間隔に抑えるために使う。
			Uint64 lastScanTick_ = 0;
		};

		/// [EN] The plugin host that owns the loaded plugins; this class only drives UserProject's rebuild-and-reload.
		/// [JP] ロード済みプラグインを所有するプラグインホスト; このクラスは UserProject の再ビルド＆リロードだけを駆動する。
		PluginHost* pluginHost_ = nullptr;

		/// [EN] The loaded UserProject plugin, cached from pluginHost_ (nullptr if not loaded).
		/// [JP] pluginHost_ からキャッシュした、ロード済みの UserProject プラグイン（未ロードなら nullptr）。
		PluginModule* userProjectPlugin_ = nullptr;

		CsharpHost* csharpHost_ = nullptr;

		/// [EN] Path to MSBuild.exe, resolved once via vswhere on first use. Empty if it could not be found.
		/// [JP] MSBuild.exe のパス。初回使用時に vswhere 経由で一度だけ解決する。見つからなければ空。
		std::filesystem::path msbuildPath_;

		/// [EN] True once msbuildPath_ resolution (success or failure) has been attempted.
		/// [JP] msbuildPath_ の解決を（成否に関わらず）一度試みたら true になる。
		Bool msbuildResolved_ = false;

		BuildTask cplusplusBuild_;

		BuildTask csharpBuild_;

		Bool csharpReloadDeferred_ = false;

		/// [EN] Launches an async MSBuild.exe build of UserProject.Cplusplus.vcxproj. No-op if MSBuild couldn't be resolved.
		/// [JP] UserProject.Cplusplus.vcxproj の非同期ビルドを MSBuild.exe で起動する。MSBuild が見つからなければ何もしない。
		void TriggerCplusplusBuild();

		void TriggerCsharpBuild();

		static Bool Launch(BuildTask& task, std::wstring commandLine, const Char* label);

		/// [EN] Reads whatever's currently buffered in the task's output pipe (non-blocking) into its output text.
		/// [JP] タスクの出力パイプに現在溜まっている分を（ノンブロッキングで）出力テキストへ読み出す。
		static void Drain(BuildTask& task);

		/// [EN] Checks whether the task's build process has finished; logs the result and, on success, sets its reloadRequested_.
		/// [JP] タスクのビルドプロセスが終了したか確認する; 結果をログに出し、成功していれば reloadRequested_ を立てる。
		static void Poll(BuildTask& task, const Char* label);

		static Bool Settled(BuildTask& task, Uint64 writeTime, Uint64 nowTick);

		[[nodiscard]] static std::filesystem::path UserProjectSourceDirectory();

		[[nodiscard]] static std::filesystem::path UserProjectVcxprojPath();

		[[nodiscard]] static std::filesystem::path UserProjectCsprojPath();

		[[nodiscard]] static Uint64 GetLastWriteTime(const std::filesystem::path& path);

		/// [EN] Recursively finds the newest last-write time among UserProject's source files (.cs when csharp is true, .h/.cpp otherwise). Returns 0 if the directory can't be scanned.
		/// [JP] UserProject 配下のソースファイル（csharp が true なら .cs、そうでなければ .h/.cpp）のうち、最も新しい最終更新時刻を再帰的に探す。ディレクトリをスキャンできない場合は 0 を返す。
		[[nodiscard]] static Uint64 ScanSourceLastWriteTime(Bool csharp);

		/// [EN] Resolves MSBuild.exe's path via vswhere.exe (a synchronous, one-time call).
		/// [JP] vswhere.exe 経由で MSBuild.exe のパスを解決する（同期・一度きりの呼び出し）。
		[[nodiscard]] static std::filesystem::path FindMSBuild();
	};
}
