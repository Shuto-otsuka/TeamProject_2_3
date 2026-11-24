#pragma once
#include <FoundationEngine/Utility/ArtMap.h>
#include <FoundationEngine/Utility/FlatMap.h>
#include <FoundationEngine/Resource/Prefab/PrefabPool.h>
#include <FoundationEngine/Resource/Scene/ScenePool.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <FoundationEngine/Resource/Asset/AxisConvention.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>

namespace SeedCore
{
	class FontManager;
	class JobExecutor;
	class BenchmarkSystem;

	/**
	* [EN]
	* Owns the project's discovered asset table and the Prefab/Scene
	* pools, and drives the resource managers each engine module
	* registers per AssetType through Register() (it does not own them).
	* Queue() scans the engine root's directory tree to (re)discover
	* assets and reconcile their .meta-derived GUIDs, then Step()/Run()
	* load them one at a time or on a background worker with progress
	* tracking, while Reload() performs a full synchronous pass.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* プロジェクトの発見済みアセットテーブルと Prefab/Scene のプールを
	* 所有し、各エンジンのモジュールが Register() で AssetType ごとに登録
	* するリソースマネージャを動かす（マネージャは所有しない）。Queue() は
	* エンジンのルートのディレクトリツリーを走査してアセットを（再）発見し、
	* .meta 由来の GUID を整合させる。その後 Step()/Run() が、1つずつ、
	* またはバックグラウンドのワーカーで、進捗を追いながら読み込む。
	* Reload() は完全な同期のパスを実行する。
	*/
	class SEEDCORE_API ResourceCache
	{
	public:
		/**
		* [EN]
		* Constructs an empty cache. Resource managers are added afterwards
		* through Register().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 空のキャッシュを構築する。リソースマネージャは後から Register() で
		* 追加する。
		*/
		ResourceCache();

		/**
		* [EN]
		* Destroys the cache through Finalize().
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Finalize() を通してキャッシュを破棄する。
		*/
		~ResourceCache();

		/**
		* [EN]
		* Waits for a running Run() job, stops the folder watch, unloads
		* every loaded asset and forgets the registered resource managers.
		* Called by the destructor, and earlier by a host whose resource
		* managers are destroyed before the cache.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 実行中の Run() のジョブを待ち、フォルダの監視を止め、読み込み済みの
		* 全アセットを解放し、登録されたリソースマネージャを忘れる。
		* デストラクタが呼ぶほか、リソースマネージャがキャッシュより先に
		* 破棄されるホストは、それより前に呼ぶ。
		*/
		void Finalize();

		/**
		* [EN]
		* Registers resource as the manager that loads and releases every
		* asset of type. The cache keeps only a reference, so resource must
		* outlive it or be dropped through Finalize() first.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* resource を、type の全アセットを読み込み・解放するマネージャとして
		* 登録する。キャッシュは参照しか持たないので、resource はキャッシュより
		* 長く生きるか、先に Finalize() で外す必要がある。
		*/
		void Register(AssetType type, Asset& resource);

		/**
		* [EN]
		* Returns the resource manager registered for type, or nullptr if
		* no manager was registered for it (e.g. Prefab/Scene, which the
		* pools handle instead).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* type に登録されているリソースマネージャを返す。登録が無ければ
		* nullptr を返す（例: Prefab/Scene は代わりにプールが扱う）。
		*/
		NODISCARD Asset* Resource(AssetType type)const;

		/**
		* [EN]
		* Returns the prefab pool.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プレハブのプールを返す。
		*/
		NODISCARD PrefabPool& Prefabs();

		/**
		* [EN]
		* Returns the scene pool.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* シーンのプールを返す。
		*/
		NODISCARD ScenePool& Scenes();

		/**
		* [EN]
		* Synchronously rescans the project for asset changes and loads
		* every not-yet-loaded asset in a single pass.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プロジェクトのアセット変更を同期的に再スキャンし、まだ読み込まれて
		* いない全アセットを一括で読み込む。
		*/
		void Reload();

		/**
		* [EN]
		* Reloads one asset whose file on disk has been replaced: releases
		* what is held in memory and loads the new contents in its place.
		* Used when the shared library brings down a newer revision while
		* the Editor is running.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ディスク上のファイルが差し替わったアセット1件を読み直す。メモリ上
		* のものを解放し、新しい中身をその場所へ読み込む。Editor の実行中に
		* 共有ライブラリが新しい Revision を持ってきた場合に使う。
		*/
		void Reload(Uint32 assetID);

		/**
		* [EN]
		* Releases one asset and drops its record, so the next scan reads its
		* file and .meta afresh. Used when the .meta on disk has been replaced
		* and now names a different identifier than the one held in memory.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセット1件を解放し、その記録を捨てる。次の走査で、ファイルと
		* .meta が改めて読み込まれるようにする。ディスク上の .meta が差し
		* 替わり、メモリ上とは別の識別子を示すようになった場合に使う。
		*/
		void Forget(Uint32 assetID);

		/**
		* [EN]
		* Watches UserProject (the engine root when UserProject does not
		* exist) for file and folder changes and runs Reload() when one has
		* happened. The first call only starts the watch, so only a host that
		* calls this every frame (the Editor) pays for it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* UserProject（UserProject が無ければエンジンのルート）のファイルと
		* フォルダの変更を監視し、変更があれば Reload() を実行する。最初の
		* 呼び出しは監視を始めるだけなので、毎フレーム呼ぶホスト（Editor）
		* だけがその負担を持つ。
		*/
		void Watch();

		/**
		* [EN]
		* Returns a counter that advances on every full Reload(), so a
		* caller that keeps the last value it saw can tell the asset list
		* has changed without being told who reloaded it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 全体の Reload() のたびに進むカウンタを返す。呼び出し側は最後に見た値を
		* 覚えておけば、誰が読み直したかを知らなくても、アセット一覧が変わった
		* ことを判断できる。
		*/
		NODISCARD Uint64 Revision()const;

		/**
		* [EN]
		* Starts (or restarts) an incremental load pass: scans the engine
		* root's directory tree on the calling thread, queues every
		* not-yet-loaded asset in dependency order, and resets the progress
		* counters. Step() or Run() then works through the queue.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 逐次の読み込みパスを開始（または再開始）する。呼び出したスレッドで
		* エンジンのルートのディレクトリツリーを走査し、まだ読み込まれていない
		* 全アセットを依存の順にキューへ積み、進捗のカウンタをリセットする。
		* その後、Step() か Run() がキューを処理する。
		*/
		void Queue();

		/**
		* [EN]
		* Loads exactly one pending asset from the queue built by Queue(),
		* if any remain, telling its manager that thread runs the load.
		* Returns whether an asset was processed this call (false once the
		* queue is drained or Queue() hasn't been called).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Queue() が構築したキューから、残っていればちょうど1つの保留
		* アセットを読み込み、その管理クラスへ thread で読み込んでいることを
		* 伝える。この呼び出しでアセットを処理したかどうかを返す（キューが
		* 尽きた、または Queue() が呼ばれていない場合は false）。
		*/
		Bool Step(LoadThread thread);

		/**
		* [EN]
		* Starts (once per Queue() pass - later calls are ignored until the
		* pass finishes) a background job that repeatedly calls Step() on a
		* dedicated worker thread until the pending queue is drained. Callers
		* poll Complete()/Progress() (both atomic) from the main thread instead
		* of calling Step() themselves, so a heavy per-asset load never blocks
		* frame presentation. Finalize() waits for this job to finish before
		* unloading anything, so it's always safe to tear down mid-load.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* バックグラウンドジョブを開始する（Queue() パスごとに1回のみ - パスが
		* 終わるまで以降の呼び出しは無視される）。専用のワーカースレッド上で
		* キューが尽きるまで Step() を繰り返し呼ぶ。呼び出し側は自分で Step()
		* を呼ぶ代わりに、メインスレッドから Complete()/Progress()（どちらも
		* atomic）をポーリングする。これにより重いアセット単位の読み込みが
		* フレーム表示をブロックすることがなくなる。Finalize() はこの
		* ジョブの完了を待ってから解放処理に入るので、読み込み中に破棄しても
		* 常に安全。
		*/
		void Run();

		/**
		* [EN]
		* Makes the dedicated asynchronous loader attach to benchmark recording.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 専用の非同期ローダーをベンチマーク計測へ Attach する。
		*/
		void Benchmark(BenchmarkSystem& benchmark);

		/**
		* [EN]
		* Returns whether the current Queue() pass has scanned and
		* loaded every pending asset.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在の Queue() パスが、保留中の全アセットをスキャン・読み込み
		* 済みかどうかを返す。
		*/
		NODISCARD Bool Complete()const;

		/**
		* [EN]
		* Returns the current load progress of the Queue() pass, from 0
		* to 1 (1 if there is nothing to load).
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Queue() パスの現在の読み込み進捗を 0 から 1 の範囲で返す
		* （読み込むものが無ければ 1）。
		*/
		NODISCARD Float Progress()const;

		/**
		* [EN]
		* Returns the asset type a file with extension (including the
		* leading dot, e.g. ".png") belongs to, or AssetType::Unknown when
		* no type uses it. This is the single place an extension is mapped
		* to a type, shared by Scan and by the team library listing.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 拡張子 extension（先頭のドットを含む。例: ".png"）のファイルが属する
		* アセットの種類を返す。どの種類も使わない拡張子なら
		* AssetType::Unknown を返す。拡張子から種類を決めるのはここだけで、
		* Scan とチームのライブラリの一覧が共有する。
		*/
		NODISCARD static AssetType Classify(const std::string& extension);

		/**
		* [EN]
		* Returns the AssetRecord registered under assetID, or nullptr if
		* unknown.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* assetID に登録されている AssetRecord を返す。不明であれば nullptr を
		* 返す。
		*/
		NODISCARD AssetRecord* Record(Uint32 assetID);

		/**
		* [EN]
		* Returns the asset ID whose path matches filename, or 0 if not
		* found. filename can be a full engine-root-relative path (e.g.
		* "UserProject/Assets/Scene/Foo.scene", matched exactly) or just a
		* bare filename (e.g. "Foo.scene", matched against every asset's
		* own filename - the caller doesn't need to know which subfolder
		* it lives in). If more than one asset shares that bare filename,
		* the first one found wins and a warning is logged, since which
		* one is "correct" is ambiguous.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パスが filename に一致するアセットの ID を返す。見つからなければ
		* 0 を返す。filename には、エンジンのルートからの相対のフルパス(例:
		* "UserProject/Assets/Scene/Foo.scene"、完全一致)か、単なる
		* ファイル名(例: "Foo.scene"、各アセット自身のファイル名と比較 -
		* 呼び出し側はどのサブフォルダにあるか知らなくてよい)のどちらも
		* 渡せる。同じファイル名を持つアセットが複数ある場合は最初に
		* 見つかったものを使い、警告を出す(どれが「正しい」かは判別
		* できないため)。
		*/
		NODISCARD Uint32 Find(String filename);

		/**
		* [EN]
		* Returns every AssetRecord whose path contains key as a substring.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* パスに key を部分文字列として含む、全ての AssetRecord を返す。
		*/
		NODISCARD DynamicArray<AssetRecord*> Search(String key);

		/**
		* [EN]
		* Returns every discovered asset's record, keyed by asset ID.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 発見済みの全アセットの記録を、アセット ID をキーとして返す。
		*/
		NODISCARD const FlatMap<Uint32, AssetRecord>& Records()const;

	private:
		/**
		* [EN]
		* Walks targetPath's directory tree, discovering asset files by
		* extension, reconciling/recovering their .meta-derived GUIDs
		* (including orphaned .meta recovery after a rename), and
		* registering new AssetRecord entries.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* targetPath のディレクトリツリーを走査し、拡張子でアセット
		* ファイルを発見し、.meta 由来の GUID を整合・復旧（リネーム後の
		* 孤立した .meta ファイルの復旧を含む）し、新しい AssetRecord エントリを
		* 登録する。
		*/
		void Scan(String targetPath);

		/**
		* [EN]
		* Removes AssetRecord entries whose backing file no longer exists
		* (unloading them first if loaded), then re-runs Scan to pick up
		* any new/changed files.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 裏付けとなるファイルがもう存在しない AssetRecord エントリを削除し
		* （読み込み済みであれば先に解放する）、その後 Scan を再実行して
		* 新規/変更されたファイルを取り込む。
		*/
		void Rescan();

		/**
		* [EN]
		* Unloads every currently-loaded asset from its owning resource
		* manager and clears the asset/search maps.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在読み込まれている全アセットを、それぞれの所有元リソース
		* マネージャから解放し、アセット/検索マップをクリアする。
		*/
		void Unload();

	private:
		/// [EN] Every discovered asset, keyed by asset ID.
		/// [JP] 発見済みの全アセット。アセット ID をキーとする。
		FlatMap<Uint32, AssetRecord> assetsMap_;

		/// [EN] Adaptive radix tree mapping an asset's path to its asset ID, for fast prefix/substring lookups.
		/// [JP] アセットのパスをそのアセット ID へ対応付ける、高速な前方一致/部分一致検索のためのアダプティブ基数木。
		ArtMap<String, Uint32> searchMap_;

		/// [EN] Resource manager per asset type, registered by each engine module through Register(); not owned, so a new asset type needs no change here.
		/// [JP] アセット種別ごとのリソースマネージャ。各エンジンのモジュールが Register() で登録する。所有はしないので、新しいアセット種別を追加してもここには変更が要らない。
		FlatMap<AssetType, Asset*> resourceMap_;

		/// [EN] Pool of loaded Prefab assets.
		/// [JP] 読み込み済みの Prefab アセットのプール。
		PrefabPool prefabPool_;

		/// [EN] Pool of loaded Scene assets.
		/// [JP] 読み込み済みの Scene アセットのプール。
		ScenePool scenePool_;

		/// [EN] AssetRecord IDs awaiting load in the current Queue() pass, in dependency order.
		/// [JP] 現在の Queue() パスで読み込みを待っている、依存の順に並んだアセット ID 群。
		DynamicArray<Uint32> pendingAssetIDs_;

		/// [EN] Index of the next pending asset for Step(); atomic because Run() updates it on a worker while Complete() reads it on the main thread.
		/// [JP] Step() が次に処理する保留アセットの番号。Run() がワーカーで更新し、Complete() がメインスレッドで読むので atomic。
		std::atomic<Size> pendingIndex_{ 0 };

		/// [EN] Total number of assets queued in the current Queue() pass.
		/// [JP] 現在の Queue() パスでキューに入れられたアセットの総数。
		std::atomic<Uint32> totalAssetCount_{ 0 };

		/// [EN] Number of assets loaded so far in the current Queue() pass.
		/// [JP] 現在の Queue() パスで、これまでに読み込まれたアセット数。
		std::atomic<Uint32> loadedAssetCount_{ 0 };

		/// [EN] Whether the directory scan for the current Queue() pass has finished.
		/// [JP] 現在の Queue() パスにおけるディレクトリスキャンが完了しているかどうか。
		std::atomic<Bool> scanComplete_{ false };

		/// [EN] Taskflow for Run()'s background job. A member, not a local, because JobTopology only references it
		///      and the job keeps running after Run() returns.
		/// [JP] Run() のバックグラウンドジョブ用の taskflow。JobTopology は参照しか持たず、
		///      ジョブは Run() が戻った後も動き続けるので、ローカル変数ではなくメンバーにする。
		JobTaskflow loadTaskflow_;

		/// [EN] Single-worker executor for Run(), created on first use and destroyed first in Finalize() (waiting for running work) before anything is unloaded.
		/// [JP] Run() 専用の1ワーカーの実行器。初回使用時に作り、Finalize() の最初で(実行中の作業を待って)何かを解放する前に破棄する。
		ResourcePtr<JobExecutor> loadExecutor_;

		/// [EN] Benchmark the dedicated load worker attaches to, when profiling is enabled.
		/// [JP] プロファイリング時に専用ローダーワーカーが Attach するベンチマーク。
		BenchmarkSystem* benchmark_ = nullptr;

		/// [EN] Guards against Run() starting a second background job while one from the current Queue() pass is still running.
		/// [JP] 現在の Queue() パスのバックグラウンドジョブがまだ実行中の間に、Run() が2つ目のジョブを開始しないようにする。
		std::atomic<Bool> loadStarted_{ false };

		/// [EN] Change-notification handle for the folder Watch() observes; INVALID_HANDLE_VALUE until Watch() is first called.
		/// [JP] Watch() が監視するフォルダの変更通知ハンドル。Watch() が初めて呼ばれるまでは INVALID_HANDLE_VALUE。
		HANDLE watchHandle_ = INVALID_HANDLE_VALUE;

		/// [EN] Number of full Reload() calls so far, returned by Revision().
		/// [JP] これまでの全体の Reload() の回数。Revision() が返す。
		Uint64 revision_ = 0;

		/// [EN] File extensions Scan recognizes as candidate assets, in AssetType order; ".navmesh" is listed with the model-derived files it is made beside but classifies as Unknown, and source code is listed last.
		/// [JP] Scan がアセット候補として認識するファイル拡張子。AssetType の順に並べる。".navmesh" は一緒に作られるモデル由来のファイルの並びに置くが、種類は Unknown になる。ソースコードは最後に置く。
		std::set<std::string_view> includeExtensions_ =
		{
			".png", ".jpg", ".jpeg", ".dds", ".texture",
			".gltf", ".glb", ".fbx", ".crister",
			".animation",
			".collision",
			".material",
			".skeleton",
			".navmesh",
			".zephyr",
			".efkefc", ".effekseer",
			".mp3", ".wav", ".acb", ".awb", ".audio",
			".ttf", ".otf", ".ttc",
			".mp4", ".movie",
			".hdr", ".skymap",
			".prefab",
			".scene",
			".h", ".cpp", ".cs", ".hlsli", ".hlsl",
		};

		/// [EN] Directory names Scan skips entirely (engine/tooling directories, not user assets).
		/// [JP] Scan が完全にスキップするディレクトリ名（ユーザーアセットではなく、エンジン/ツール用ディレクトリ）。
		std::set<std::string_view> excludeDirectories_ =
		{
			"AIEngine",
			"AudioEngine",
			"CompiledShaderObject",
			"Editor",
			"External",
			"FoundationEngine",
			"GraphicsEngine",
			"Launcher",
			"Logs",
			"Package",
			"PhysicsEngine",
			"Platform",
			"Runtime",
			"SeedCore",
			"Tools",
			".vs",
			"x64",
			".git",
			".asset",
		};

		/// [EN] File extensions that don't get a companion .meta file (their GUID is derived from a path hash instead).
		/// [JP] 対応する .meta ファイルを持たないファイル拡張子（その GUID は代わりにパスハッシュから導出される）。
		std::set<std::string_view> noMetaExtensions_ =
		{
			".h", 
			".cpp", 
			".cs", 
			".hlsli", 
			".hlsl"
		};

		/// [EN] Extensions of the files the engine writes out by itself, in AssetType order. Their .meta is still written, but the GUID in it is derived from the project-relative path, so every machine that bakes the same file arrives at the same identifier without exchanging anything.
		/// [JP] エンジンが自分で書き出すファイルの拡張子。AssetType の順に並べる。.meta は書き出すが、その GUID はプロジェクト内の位置から導出する。同じファイルを焼いた PC は、何もやり取りせずに同じ識別子へ行き着く。
		std::set<std::string_view> generatedExtensions_ =
		{
			".texture",
			".crister",
			".animation",
			".collision",
			".material",
			".skeleton",
			".navmesh",
			".audio",
			".movie",
			".skymap",
		};
	};
}
