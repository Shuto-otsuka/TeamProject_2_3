#include <FoundationEngine/Resource/ResourceCache.h>
#include <FoundationEngine/Benchmark/Benchmark.h>
#include <FoundationEngine/Benchmark/BenchmarkSystem.h>
#include <FoundationEngine/JobSystem/JobScheduler.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>
#include <FoundationEngine/File/FileLocator.h>

namespace SeedCore
{
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
	ResourceCache::ResourceCache()
	{
		/// No Code
	}

	/**
	* [EN]
	* Destroys the cache through Finalize().
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Finalize() を通してキャッシュを破棄する。
	*/
	ResourceCache::~ResourceCache()
	{
		Finalize();
	}

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
	void ResourceCache::Finalize()
	{
		/// [EN] Destroying the executor first waits (in ~JobExecutor) for any running Run() job, so the cache can be destroyed mid-load.
		/// [JP] 先に実行器を破棄し、動いている Run() のジョブの終了を(~JobExecutor で)待つ。これで読み込み中でもキャッシュを破棄できる。
		loadExecutor_ = nullptr;

		/// [EN] The watch exists only once Watch() has been called, which the Runtime never does.
		/// [JP] 監視は Watch() が呼ばれて初めて作られる。Runtime は一度も呼ばない。
		if (watchHandle_ != INVALID_HANDLE_VALUE)
		{
			FindCloseChangeNotification(watchHandle_);
			watchHandle_ = INVALID_HANDLE_VALUE;
		}

		Unload();

		/// [EN] Finalize also runs from the destructor after a host already called it, so the managers are forgotten here to make that second call release nothing through them.
		/// [JP] Finalize は、ホストが呼んだ後にデストラクタからも走るので、ここでマネージャを忘れ、2回目の呼び出しがマネージャを通して何も解放しないようにする。
		resourceMap_.clear();
	}

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
	void ResourceCache::Register(AssetType type, Asset& resource)
	{
		resourceMap_.insert({ type, &resource });
	}

	/**
	* [EN]
	* Returns the resource manager registered for type, or nullptr if no
	* manager was registered for it (e.g. Prefab/Scene, which the pools
	* handle instead).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* type に登録されているリソースマネージャを返す。登録が無ければ
	* nullptr を返す（例: Prefab/Scene は代わりにプールが扱う）。
	*/
	Asset* ResourceCache::Resource(AssetType type)const
	{
		if (!resourceMap_.contains(type))
		{
			return nullptr;
		}

		return resourceMap_.at(type);
	}

	/**
	* [EN]
	* Returns the prefab pool.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* プレハブのプールを返す。
	*/
	PrefabPool& ResourceCache::Prefabs()
	{
		return prefabPool_;
	}

	/**
	* [EN]
	* Returns the scene pool.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* シーンのプールを返す。
	*/
	ScenePool& ResourceCache::Scenes()
	{
		return scenePool_;
	}

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
	void ResourceCache::Reload()
	{
		Rescan();

		AssetContext context{ *this, LoadThread::Main };

		for (AssetRecord& asset : assetsMap_ | std::ranges::views::values)
		{
			if (asset.isLoaded_ || asset.type_ == AssetType::Unknown)
			{
				continue;
			}

			Asset* resource = Resource(asset.type_);
			if (resource)
			{
				resource->Load(context, asset.assetID_);
			}

			asset.isLoaded_ = true;
		}

		/// [EN] This scan already covers every change signalled so far, so drain the watch; each FindNextChangeNotification re-signals at once while changes are still queued, hence the loop.
		/// [JP] ここまでに通知された変更は今回の走査で全て反映済みなので、監視の通知を空にする。変更が溜まっている間は FindNextChangeNotification がすぐ再通知するため、ループで回す。
		while (watchHandle_ != INVALID_HANDLE_VALUE && WaitForSingleObject(watchHandle_, 0) == WAIT_OBJECT_0)
		{
			FindNextChangeNotification(watchHandle_);
		}

		/// [EN] Advance last, once the asset list is final, so a caller rebuilding from it sees the finished state.
		/// [JP] アセット一覧が確定してから最後に進める。それを見て作り直す呼び出し側が、完成した状態を見られるようにするため。
		++revision_;
	}

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
	void ResourceCache::Reload(Uint32 assetID)
	{
		/// [EN] An asset this cache has never seen has nothing to release, and its file is picked up by the next scan instead.
		/// [JP] このキャッシュが知らないアセットには解放するものが無く、そのファイルは次の走査で拾われる。
		auto it = assetsMap_.find(assetID);
		if (it == assetsMap_.end() || it->second.type_ == AssetType::Unknown)
		{
			return;
		}

		Asset* resource = Resource(it->second.type_);
		if (!resource)
		{
			return;
		}

		/// [EN] The identifier survives the swap, so every reference held elsewhere keeps pointing at this asset.
		/// [JP] 識別子は入れ替えをまたいで変わらないため、他所が持っている参照はこのアセットを指したままになる。
		AssetContext context{ *this, LoadThread::Main };
		if (it->second.isLoaded_)
		{
			resource->Unload(assetID);
			it->second.isLoaded_ = false;
		}

		resource->Load(context, assetID);
		it->second.isLoaded_ = true;
	}

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
	void ResourceCache::Forget(Uint32 assetID)
	{
		auto it = assetsMap_.find(assetID);
		if (it == assetsMap_.end())
		{
			return;
		}

		/// [EN] What is loaded under the old identifier is released first, since nothing will ever ask for it by that identifier again.
		/// [JP] 古い識別子で読み込まれているものを先に解放する。以後その識別子で求められることは無いため。
		if (it->second.isLoaded_ && it->second.type_ != AssetType::Unknown)
		{
			Asset* resource = Resource(it->second.type_);
			if (resource)
			{
				resource->Unload(assetID);
			}
		}

		/// [EN] A scan skips paths it already knows, so dropping the record is what makes it read the replaced .meta.
		/// [JP] 走査は既に知っている位置を飛ばす。記録を捨てることが、差し替わった .meta を読ませることにつながる。
		assetsMap_.erase(it);
	}

	/**
	* [EN]
	* Starts the folder watch on the first call, and on later calls runs
	* Reload() once a change has been signalled.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 最初の呼び出しでフォルダの監視を始め、以降の呼び出しでは変更が通知されて
	* いれば Reload() を実行する。
	*/
	void ResourceCache::Watch()
	{
		/// [EN] Gameplay assets live under UserProject; watching the whole root would also catch build output and Visual Studio's own files.
		/// [JP] ゲームのアセットは UserProject 以下にある。ルート全体を監視すると、ビルド出力や Visual Studio 自身のファイルまで拾ってしまう。
		if (watchHandle_ == INVALID_HANDLE_VALUE)
		{
			std::filesystem::path watchPath = FileLocator::ProjectRoot();
			if (!std::filesystem::exists(watchPath))
			{
				watchPath = FileLocator::EngineRoot();
			}
			watchHandle_ = FindFirstChangeNotificationW(watchPath.wstring().c_str(), TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
			return;
		}

		/// [EN] A zero timeout only checks the signal, so an idle frame never waits here.
		/// [JP] タイムアウト 0 は通知の有無を見るだけなので、何も変わっていないフレームがここで待つことはない。
		if (WaitForSingleObject(watchHandle_, 0) == WAIT_OBJECT_0)
		{
			Reload();
		}
	}

	/**
	* [EN]
	* Returns the number of full Reload() calls so far.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* これまでの全体の Reload() の回数を返す。
	*/
	Uint64 ResourceCache::Revision()const
	{
		return revision_;
	}

	/**
	* [EN]
	* Starts (or restarts) an incremental load pass: scans the engine
	* root's directory tree on the calling thread, queues every
	* not-yet-loaded asset in dependency order, and resets the progress
	* counters.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 逐次の読み込みパスを開始（または再開始）する。呼び出したスレッドで
	* エンジンのルートのディレクトリツリーを走査し、まだ読み込まれていない
	* 全アセットを依存の順にキューへ積み、進捗のカウンタをリセットする。
	*/
	void ResourceCache::Queue()
	{
		scanComplete_.store(false, std::memory_order_release);
		pendingAssetIDs_.clear();
		pendingIndex_ = 0;
		totalAssetCount_.store(0, std::memory_order_release);
		loadedAssetCount_.store(0, std::memory_order_release);
		loadStarted_.store(false, std::memory_order_release);

		Scan(String(FileLocator::EngineRoot().wstring()));

		/// [EN] Load order matters: textures/models/animations before the things that reference them (e.g. effects/audio after visuals), scenes/prefabs last since they may reference everything else.
		/// [JP] 読み込み順序が重要: テクスチャ/モデル/アニメーションを、それらを参照するもの（ビジュアルの後にエフェクト/オーディオなど）より先に読み込む。シーン/プレハブは他の全てを参照し得るため最後にする。
		static const AssetType order[] =
		{
			AssetType::Texture,
			AssetType::Model,
			AssetType::Animation,
			AssetType::MeshCollision,
			AssetType::Material,
			AssetType::Skeleton,
			AssetType::Font,
			AssetType::Effekseer,
			AssetType::Effect,
			AssetType::Audio,
			AssetType::Movie,
			AssetType::Prefab,
			AssetType::Scene,
			AssetType::Skymap,
		};

		for (AssetType type : order)
		{
			for (const AssetRecord& asset : assetsMap_ | std::ranges::views::values)
			{
				if (asset.type_ == type && !asset.isLoaded_)
				{
					pendingAssetIDs_.push_back(asset.assetID_);
				}
			}
		}

		totalAssetCount_.store(static_cast<Uint32>(pendingAssetIDs_.size()), std::memory_order_release);
		scanComplete_.store(true, std::memory_order_release);
	}

	/**
	* [EN]
	* Loads exactly one pending asset from the queue built by Queue(), if
	* any remain, telling its manager that thread runs the load. Returns
	* whether an asset was processed this call (false once the queue is
	* drained or Queue() hasn't been called).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Queue() が構築したキューから、残っていればちょうど1つの保留
	* アセットを読み込み、その管理クラスへ thread で読み込んでいることを
	* 伝える。この呼び出しでアセットを処理したかどうかを返す（キューが
	* 尽きた、または Queue() が呼ばれていない場合は false）。
	*/
	Bool ResourceCache::Step(LoadThread thread)
	{
		if (!scanComplete_.load(std::memory_order_acquire))
		{
			return false;
		}

		if (pendingIndex_ >= pendingAssetIDs_.size())
		{
			return false;
		}

		Uint32 assetID = pendingAssetIDs_[pendingIndex_++];

		AssetRecord* asset = Record(assetID);
		if (asset && !asset->isLoaded_)
		{
			/// [EN] Dispatch to the manager for this asset type (Prefab/Scene have no Step loader). The thread is passed along so a manager keeping per-thread state never shares it with a load running on the main thread at the same time.
			/// [JP] アセット種別に対応する管理クラスへ渡す(Prefab/Scene は Step の読み込みを持たない)。スレッドの種類も渡し、スレッドごとの状態を持つ管理クラスが、同時にメインスレッドで動く読み込みとそれを共有しないようにする。
			Asset* resource = Resource(asset->type_);
			if (resource)
			{
				AssetContext context{ *this, thread };
				Benchmark::Begin("Load");
				resource->Load(context, asset->assetID_);
				Benchmark::End("Load");
			}

			asset->isLoaded_ = true;
		}

		loadedAssetCount_.fetch_add(1, std::memory_order_acq_rel);
		return true;
	}

	/**
	* [EN]
	* Starts (once per Queue() pass) a background job that repeatedly
	* calls Step() on a dedicated worker thread until the pending queue is
	* drained. See the header doc comment for the full contract.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* バックグラウンドジョブを開始する（Queue() パスごとに1回）。専用の
	* ワーカースレッド上でキューが尽きるまで Step() を繰り返し呼ぶ。
	* 完全な契約についてはヘッダのドキュメントコメント参照。
	*/
	void ResourceCache::Run()
	{
		Bool expected = false;
		if (!loadStarted_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
		{
			return;
		}

		if (!loadExecutor_)
		{
			ResourceRef<JobWorkerInterface> scheduler = benchmark_ ? MakeWorkerInterface<JobScheduler>(*benchmark_) : nullptr;
			loadExecutor_ = MakePtr<JobExecutor>(1, scheduler);
		}

		/// [EN] Clear the member taskflow first, since a new pass (Queue() resetting loadStarted_) would otherwise re-run it with the previous pass's finished task still inside.
		/// [JP] 先にメンバーの taskflow を空にする。新しいパス(Queue() が loadStarted_ を戻した場合)で、前回の完了済みタスクが残ったまま再実行しないため。
		loadTaskflow_.Clear();
		loadTaskflow_.emplace([this]()
		{
			while (Step(LoadThread::Async))
			{
				/// No Code
			}
		});

		/// [EN] Passed by reference, not moved: the JobTopology only keeps a reference to the taskflow, which the member loadTaskflow_ keeps alive.
		/// [JP] ムーブせず参照で渡す。JobTopology は taskflow への参照しか持たず、その寿命はメンバーの loadTaskflow_ が保つ。
		loadExecutor_->Run(loadTaskflow_);
	}

	/**
	* [EN]
	* Makes the dedicated asynchronous loader attach to benchmark recording.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 専用の非同期ローダーをベンチマーク計測へ Attach する。
	*/
	void ResourceCache::Benchmark(BenchmarkSystem& benchmark)
	{
		benchmark_ = &benchmark;
	}

	/**
	* [EN]
	* Returns whether the current Queue() pass has scanned and loaded
	* every pending asset.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在の Queue() パスが、保留中の全アセットをスキャン・読み込み済みか
	* どうかを返す。
	*/
	Bool ResourceCache::Complete()const
	{
		return scanComplete_.load(std::memory_order_acquire) && pendingIndex_ >= pendingAssetIDs_.size();
	}

	/**
	* [EN]
	* Returns the current load progress of the Queue() pass, from 0 to 1
	* (1 if there is nothing to load).
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Queue() パスの現在の読み込み進捗を 0 から 1 の範囲で返す
	* （読み込むものが無ければ 1）。
	*/
	Float ResourceCache::Progress()const
	{
		if (!scanComplete_.load(std::memory_order_acquire))
		{
			return 0.0f;
		}

		Uint32 total = totalAssetCount_.load(std::memory_order_acquire);
		if (total == 0)
		{
			return 1.0f;
		}

		return static_cast<Float>(loadedAssetCount_.load(std::memory_order_acquire)) / static_cast<Float>(total);
	}

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
	AssetType ResourceCache::Classify(const std::string& extension)
	{
		/// [EN] Images, and the ".texture" cache baked from them.
		/// [JP] 画像と、そこから焼いた ".texture" キャッシュ。
		if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".dds" || extension == ".texture")
		{
			return AssetType::Texture;
		}

		/// [EN] Model sources, and the ".crister" cache baked from them.
		/// [JP] モデルのソースと、そこから焼いた ".crister" キャッシュ。
		if (extension == ".gltf" || extension == ".glb" || extension == ".fbx" || extension == ".crister")
		{
			return AssetType::Model;
		}

		/// [EN] Files derived from a model: animation clips, collision geometry, materials and skeleton rigs.
		/// [JP] モデルから作るファイル。アニメーションクリップ、衝突ジオメトリ、マテリアル、スケルトンリグ。
		if (extension == ".animation")
		{
			return AssetType::Animation;
		}
		if (extension == ".collision")
		{
			return AssetType::MeshCollision;
		}
		if (extension == ".material")
		{
			return AssetType::Material;
		}
		if (extension == ".skeleton")
		{
			return AssetType::Skeleton;
		}

		/// [EN] Engine-made effects are ".zephyr"; Effekseer effects are the ".efkefc" source and the ".effekseer" cache baked from it.
		/// [JP] エンジン製のエフェクトは ".zephyr"。Effekseer のエフェクトは ".efkefc" のソースと、そこから焼いた ".effekseer" キャッシュ。
		if (extension == ".zephyr")
		{
			return AssetType::Effect;
		}
		if (extension == ".efkefc" || extension == ".effekseer")
		{
			return AssetType::Effekseer;
		}

		/// [EN] Audio sources, CRI ADX2 banks, and the ".audio" cache.
		/// [JP] 音声のソース、CRI ADX2 のバンク、".audio" キャッシュ。
		if (extension == ".mp3" || extension == ".wav" || extension == ".acb" || extension == ".awb" || extension == ".audio")
		{
			return AssetType::Audio;
		}
		if (extension == ".ttf" || extension == ".otf" || extension == ".ttc")
		{
			return AssetType::Font;
		}
		if (extension == ".mp4" || extension == ".movie")
		{
			return AssetType::Movie;
		}
		if (extension == ".hdr" || extension == ".skymap")
		{
			return AssetType::Skymap;
		}

		/// [EN] Prefabs and scenes, which reference the assets above.
		/// [JP] 上のアセットを参照するプレハブとシーン。
		if (extension == ".prefab")
		{
			return AssetType::Prefab;
		}
		if (extension == ".scene")
		{
			return AssetType::Scene;
		}
		return AssetType::Unknown;
	}

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
	AssetRecord* ResourceCache::Record(Uint32 assetID)
	{
		auto it = assetsMap_.find(assetID);
		if (it != assetsMap_.end())
		{
			return &(it->second);
		}
		return nullptr;
	}

	/**
	* [EN]
	* Returns the asset ID whose path matches filename, or 0 if not
	* found. filename can be a full engine-root-relative path (e.g.
	* "UserProject/Assets/Scene/Foo.scene", matched exactly) or just a
	* bare filename (e.g. "Foo.scene", matched against every asset's own
	* filename - the caller doesn't need to know which subfolder it
	* lives in). If more than one asset shares that bare filename, the
	* first one found wins and a warning is logged, since which one is
	* "correct" is ambiguous.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* パスが filename に一致するアセットの ID を返す。見つからなければ
	* 0 を返す。filename には、エンジンのルートからの相対のフルパス(例:
	* "UserProject/Assets/Scene/Foo.scene"、完全一致)か、単なるファイル名
	* (例: "Foo.scene"、各アセット自身のファイル名と比較 - 呼び出し側は
	* どのサブフォルダにあるか知らなくてよい)のどちらも渡せる。同じ
	* ファイル名を持つアセットが複数ある場合は最初に見つかったものを使い、
	* 警告を出す(どれが「正しい」かは判別できないため)。
	*/
	Uint32 ResourceCache::Find(String filename)
	{
		if (searchMap_.contains(filename))
		{
			return searchMap_.at(filename);
		}

		std::string targetFilename = std::filesystem::path(filename.c_str()).filename().string();

		Uint32 matchedAssetID = 0;
		Uint32 matchCount = 0;
		for (const AssetRecord& asset : assetsMap_ | std::ranges::views::values)
		{
			std::string candidateFilename = std::filesystem::path(asset.path_.c_str()).filename().string();
			if (candidateFilename != targetFilename)
			{
				continue;
			}

			if (matchCount == 0)
			{
				matchedAssetID = asset.assetID_;
			}
			++matchCount;
		}

		if (matchCount > 1)
		{
			SC_LOG_WARNING("ファイル名 \"{}\" のアセットが複数見つかりました。最初に見つかったものを使います。", filename.c_str());
		}

		return matchedAssetID;
	}

	/**
	* [EN]
	* Returns every AssetRecord whose path contains key as a substring.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* パスに key を部分文字列として含む、全ての AssetRecord を返す。
	*/
	DynamicArray<AssetRecord*> ResourceCache::Search(String key)
	{
		DynamicArray<AssetRecord*> result;

		for (AssetRecord& asset : assetsMap_ | std::ranges::views::values)
		{
			if (asset.path_.str().contains(key.c_str()))
			{
				result.push_back(&asset);
			}
		}

		return result;
	}

	/**
	* [EN]
	* Returns every discovered asset's record, keyed by asset ID.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 発見済みの全アセットの記録を、アセット ID をキーとして返す。
	*/
	const FlatMap<Uint32, AssetRecord>& ResourceCache::Records()const
	{
		return assetsMap_;
	}

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
	* targetPath のディレクトリツリーを走査し、拡張子でアセットファイルを
	* 発見し、.meta 由来の GUID を整合・復旧（リネーム後の孤立した .meta
	* ファイルの復旧を含む）し、新しい AssetRecord エントリを登録する。
	*/
	void ResourceCache::Scan(String targetPath)
	{
		std::filesystem::path root(targetPath.c_str());
		if (!std::filesystem::exists(root))
		{
			return;
		}

		/// [EN] Remember which paths are already registered, so the second pass below only adds genuinely new assets.
		/// [JP] 既に登録済みのパスを記憶しておく。これにより、以下の第二パスでは本当に新しいアセットのみを追加する。
		std::unordered_set<std::string> existingPaths;
		for (const AssetRecord& asset : assetsMap_ | std::ranges::views::values)
		{
			existingPaths.insert(asset.path_.str());
		}

		/// [EN] Orphaned .meta files by the filename of the asset they belonged to, each mapped to that asset's former path (the .meta path without its ".meta").
		/// [JP] 孤立した .meta を、持ち主だったアセットのファイル名で引けるようにしたもの。値はそのアセットの元の位置（.meta の位置から ".meta" を除いたもの）。
		std::unordered_map<std::string, std::filesystem::path> orphanedMetas;
		std::unordered_set<std::string> ambiguousOrphanNames;

		/// [EN] First pass: find "orphaned" .meta files (a .meta with no corresponding asset file anymore, likely because the asset was renamed) so their GUID can be recovered instead of minted fresh. A filename appearing more than once is ambiguous and excluded, since we can't tell which orphan matches which renamed file.
		/// [JP] 第一パス: 「孤立した」.meta ファイル（対応するアセットファイルがもう存在しない、おそらくアセットがリネームされたもの）を見つける。これにより、新規に発行するのではなくその GUID を復旧できるようにする。同じファイル名が複数回現れる場合は曖昧なため除外する。どの孤立ファイルがどのリネーム後ファイルに対応するか判別できないため。
		for (auto it = std::filesystem::recursive_directory_iterator(root); it != std::filesystem::recursive_directory_iterator(); ++it)
		{
			if (it->is_directory())
			{
				std::string directoryName = it->path().filename().string();
				if (excludeDirectories_.contains(directoryName))
				{
					it.disable_recursion_pending();
				}
				continue;
			}

			const std::filesystem::directory_entry& entry = *it;
			if (!entry.is_regular_file() || entry.path().extension().string() != ".meta")
			{
				continue;
			}

			std::filesystem::path formerPath = entry.path();
			formerPath.replace_extension();

			if (std::filesystem::exists(formerPath))
			{
				continue;
			}

			std::string filename = formerPath.filename().string();
			if (ambiguousOrphanNames.contains(filename))
			{
				continue;
			}

			if (orphanedMetas.contains(filename))
			{
				orphanedMetas.erase(filename);
				ambiguousOrphanNames.insert(filename);
			}
			else
			{
				orphanedMetas.insert({ filename, formerPath });
			}
		}

		/// [EN] Second pass: discover actual asset files by extension and register any not already known.
		/// [JP] 第二パス: 拡張子で実際のアセットファイルを発見し、まだ知られていないものを登録する。
		for (auto it = std::filesystem::recursive_directory_iterator(root); it != std::filesystem::recursive_directory_iterator(); ++it)
		{
			if (it->is_directory())
			{
				std::string directoryName = it->path().filename().string();
				if (excludeDirectories_.contains(directoryName))
				{
					it.disable_recursion_pending();
				}
				continue;
			}

			const std::filesystem::directory_entry& entry = *it;

			if (!entry.is_regular_file())
			{
				continue;
			}

			std::string extension = entry.path().extension().string();

			if (!includeExtensions_.contains(extension))
			{
				continue;
			}

			std::string rootPath = std::filesystem::relative(entry.path(), root).string();
			std::ranges::replace(rootPath, '\\', '/');

			if (existingPaths.contains(rootPath))
			{
				continue;
			}

			AssetRecord asset;

			std::string fullPath = std::filesystem::absolute(entry.path()).string();
			std::ranges::replace(fullPath, '\\', '/');
			asset.fullpath_ = String(fullPath);

			asset.path_ = String(rootPath);

			/// [EN] Classify the asset's type from its extension.
			/// [JP] 拡張子からアセットの種別を分類する。
			asset.type_ = Classify(extension);

			AssetMeta meta;
			Bool skipMeta = noMetaExtensions_.contains(extension);

			/// [EN] A generated file takes its GUID from its path alone, so every machine that bakes it arrives at the same identifier, while anything a member made gets the time mixed in so a file recreated at the same place is a different asset. A GUID already taken is bumped until it is free.
			/// [JP] 生成ファイルは位置だけから GUID を取る。焼いたどの PC も同じ識別子に行き着くようにするため。メンバーが作ったものには時刻を混ぜ、同じ位置に作り直したファイルを別のアセットとして扱う。既に使われている GUID は、空くまで1つずつ進める。
			auto mint = [this, &asset, &extension]()
				{
					std::string seed = asset.path_.str();
					if (!generatedExtensions_.contains(extension))
					{
						seed += std::to_string(std::time(nullptr));
					}
					Uint32 guid = static_cast<Uint32>(std::hash<std::string>{}(seed));
					while (assetsMap_.contains(guid))
					{
						guid++;
					}
					return guid;
				};

			if (skipMeta)
			{
				/// [EN] This extension never gets a .meta file: derive a GUID from the path hash instead, bumping on collision.
				/// [JP] この拡張子は .meta ファイルを持たない: 代わりにパスハッシュから GUID を導出し、衝突時はインクリメントする。
				meta.guid_ = static_cast<Uint32>(std::hash<std::string>{}(asset.path_.str()));
				while (assetsMap_.contains(meta.guid_))
				{
					meta.guid_++;
				}
			}
			else
			{
				std::filesystem::path metaPath = entry.path();
				metaPath += ".meta";
				if (std::filesystem::exists(metaPath))
				{
					/// [EN] A .meta already exists: read its GUID; if that fails and a new GUID is minted, rewrite the file so the next scan does not repeat the repair.
					/// [JP] .meta が既にあるのでその GUID を読む。読めずに新しい GUID を発行した場合は、次のスキャンで同じ修復を繰り返さないよう書き直す。
					if (!meta.Read(entry.path()))
					{
						meta = AssetMeta{};
						meta.guid_ = mint();
						meta.Write(entry.path());
					}
				}
				else
				{
					/// [EN] No .meta file yet: check whether an orphaned .meta from a same-named file (i.e. this file was likely renamed/moved) can be recovered and relocated here, preserving its GUID.
					/// [JP] まだ .meta ファイルが無い: 同名のファイルから孤立した .meta（すなわちこのファイルがリネーム/移動された可能性がある）を復旧し、この場所へ再配置してその GUID を維持できるか確認する。
					auto orphanIt = orphanedMetas.find(entry.path().filename().string());
					Bool recovered = false;

					if (orphanIt != orphanedMetas.end())
					{
						/// [EN] Reads the orphaned .meta so the renamed asset recovers its original GUID instead of being minted a new one.
						/// [JP] 孤立した .meta を読み取り、リネームされたアセットが新規 GUID を発行されるのではなく元の GUID を復旧するようにする。
						if (meta.Read(orphanIt->second))
						{
							/// [EN] Move the recovered .meta file to sit alongside the renamed asset; fall back to copy+delete if the rename fails (e.g. across volumes).
							/// [JP] 復旧した .meta ファイルを、リネームされたアセットの隣へ移動する。リネームが失敗した場合（異なるボリューム間など）はコピー＋削除にフォールバックする。
							std::filesystem::path orphanMetaPath = orphanIt->second;
							orphanMetaPath += ".meta";
							std::error_code errorCode;
							std::filesystem::rename(orphanMetaPath, metaPath, errorCode);
							if (errorCode)
							{
								std::filesystem::copy_file(orphanMetaPath, metaPath, std::filesystem::copy_options::overwrite_existing, errorCode);
								std::filesystem::remove(orphanMetaPath, errorCode);
							}

							recovered = true;
						}

						orphanedMetas.erase(orphanIt);
					}

					if (!recovered)
					{
						/// [EN] Genuinely new asset (or recovery failed): mint a fresh GUID and persist a new .meta file for it.
						/// [JP] 本当に新規のアセット（または復旧に失敗した場合）: 新しい GUID を発行し、そのための新しい .meta ファイルを永続化する。
						meta.guid_ = mint();
						meta.Write(entry.path());
					}
				}
			}

			asset.assetID_ = meta.guid_;

			assetsMap_.insert({ asset.assetID_, asset });
			searchMap_.insert(asset.path_, asset.assetID_);
		}
	}

	/**
	* [EN]
	* Removes AssetRecord entries whose backing file no longer exists
	* (unloading them first if loaded), then re-runs Scan to pick up any
	* new/changed files.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 裏付けとなるファイルがもう存在しない AssetRecord エントリを削除し
	* （読み込み済みであれば先に解放する）、その後 Scan を再実行して
	* 新規/変更されたファイルを取り込む。
	*/
	void ResourceCache::Rescan()
	{
		for (auto it = assetsMap_.begin(); it != assetsMap_.end();)
		{
			if (!std::filesystem::exists(it->second.fullpath_.c_str()))
			{
				if (it->second.isLoaded_)
				{
					Asset* resource = Resource(it->second.type_);
					if (resource)
					{
						resource->Unload(it->second.assetID_);
					}
				}
				it = assetsMap_.erase(it);
			}
			else
			{
				++it;
			}
		}

		/// [EN] Rebuild the search index from the surviving assets before re-scanning for new ones.
		/// [JP] 新規アセットを再スキャンする前に、生き残ったアセットから検索インデックスを再構築する。
		searchMap_.clear();
		for (const AssetRecord& asset : assetsMap_ | std::ranges::views::values)
		{
			searchMap_.insert(asset.path_, asset.assetID_);
		}

		Scan(String(FileLocator::EngineRoot().wstring()));
	}

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
	void ResourceCache::Unload()
	{
		for (AssetRecord& asset : assetsMap_ | std::ranges::views::values)
		{
			if (!asset.isLoaded_ || asset.type_ == AssetType::Unknown)
			{
				continue;
			}

			Asset* resource = Resource(asset.type_);
			if (resource)
			{
				resource->Unload(asset.assetID_);
			}

			asset.isLoaded_ = false;
		}

		for (Asset* resource : resourceMap_ | std::ranges::views::values)
		{
			resource->Clear();
		}

		assetsMap_.clear();
		searchMap_.clear();
	}
}
