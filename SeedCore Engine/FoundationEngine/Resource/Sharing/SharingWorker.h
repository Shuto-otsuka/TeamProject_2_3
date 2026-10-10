#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Resource/Sharing/GoogleAuth.h>
#include <FoundationEngine/Resource/Sharing/GoogleDocument.h>
#include <FoundationEngine/Resource/Sharing/GoogleDrive.h>
#include <FoundationEngine/Resource/Sharing/HttpClient.h>
#include <FoundationEngine/Resource/Sharing/SharedCatalog.h>

namespace SeedCore
{
	/**
	* [EN]
	* Everything the sharing layer needs that the team has to agree on
	* beforehand, read from the project's own configuration file.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有機能が必要とするもののうち、チームで事前に決めておく必要がある
	* 設定。プロジェクト内の設定ファイルから読み込む。
	*/
	struct SharingConfig
	{
		/// [EN] OAuth client of the Editor as registered with Google.
		/// [JP] Google に登録した、Editor としての OAuth クライアント。
		String clientId_;
		String clientSecret_;

		/// [EN] The document holding the catalog of shared assets.
		/// [JP] 共有アセットのカタログを保持しているドキュメント。
		String catalogDocumentId_;

		/// [EN] The Drive folder that stores asset contents, one file per content hash.
		/// [JP] アセットの中身を保存する Drive のフォルダ。中身のハッシュごとに1ファイル。
		String blobFolderId_;

		/// [EN] The Drive folder that mirrors the workspace's Assets tree, so an artist can drop a file straight into the folder it belongs in.
		/// [JP] ワークスペースの Assets のフォルダ構成を映した Drive のフォルダ。アーティストが、属するフォルダへそのまま置けるようにするため。
		String assetsFolderId_;

		/// [EN] Folder inside the project that shared content belongs to, normally "UserProject".
		/// [JP] 共有対象の内容が属する、プロジェクト内のフォルダ。通常は "UserProject"。
		String workspace_;
	};

	/**
	* [EN]
	* One file of a shared asset as this machine last fetched or published
	* it. Comparing the file on disk against this is how a local change is
	* told apart from an untouched copy.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有アセットのファイル1つを、この PC が最後に取得・公開した時点の
	* 姿で持つ。ディスク上のファイルをこれと突き合わせることで、手を
	* 加えた写しと無変更の写しを区別する。
	*/
	struct WorkspaceFile
	{
		/// [EN] Where the file sits inside the workspace.
		/// [JP] ワークスペース内でのそのファイルの位置。
		String path_;

		/// [EN] What its contents hashed to when it was fetched or published.
		/// [JP] 取得・公開した時点で、その中身がどのハッシュだったか。
		String hash_;

		/// [EN] Its size and last-write time at that same moment, which a check can compare before hashing anything.
		/// [JP] 同じ時点でのサイズと最終更新時刻。ハッシュを計算する前の比較に使える。

		/// [EN] Re-hashing every shared file on a timer would read gigabytes from disk; these two rule most files out first.
		/// [JP] 定期的に全共有ファイルをハッシュし直すとギガバイト単位の読み込みになる。この2つが、ほとんどのファイルを先に除外する。
		Uint64 size_ = 0;
		Int64 stamp_ = 0;
	};

	/**
	* [EN]
	* What this machine holds for one shared asset: which revision it was
	* fetched or published at, and every one of its files as they were at
	* that moment. Only assets with a record are followed by the latest
	* fetch.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* この PC が共有アセット1件について保持している内容。どの Revision で
	* 取得・公開したかと、その時点での各ファイルの姿。記録を持つアセット
	* だけが、最新の取得で追いかけられる。
	*/
	struct WorkspaceRecord
	{
		/// [EN] The revision this machine last fetched or published, which tells whether the library has moved ahead since.
		/// [JP] この PC が最後に取得・公開した Revision。その後ライブラリが先へ進んだかどうかの判断に使う。
		Uint64 revision_ = 0;

		/// [EN] Every file of the asset as this machine last saw it.
		/// [JP] そのアセットの各ファイルを、この PC が最後に見た時点の姿で持つ。
		DynamicArray<WorkspaceFile> files_;
	};

	/**
	* [EN]
	* What the worker wants done. Requests are handed over from the Editor
	* thread and carried out in the background, since each one is a
	* network round trip that must not stall drawing.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ワーカーに依頼する内容。Editor のスレッドから渡され、裏で実行される。
	* どれも通信を伴い、描画を止めてはならないため。
	*/
	enum class SharingAction
	{
		/// [EN] Re-read the catalog, then bring down whatever the library moved ahead on.
		/// [JP] カタログを読み直し、ライブラリが先へ進んだものを降ろす。
		Refresh,

		/// [EN] Bring the local copy up to the catalog's revision.
		/// [JP] ローカルの写しを、カタログの Revision まで引き上げる。
		Get,

		/// [EN] Replace the local copy with the library's, including its .meta, even where the local files differ.
		/// [JP] ローカルの写しを、.meta も含めてライブラリのもので置き換える。ローカルのファイルが異なっていても置き換える。
		Adopt,

		/// [EN] Share a local asset for the first time.
		/// [JP] ローカルのアセットを初めて共有する。
		Register,

		/// [EN] Send local changes to the team as a new revision, replacing whatever the library held.
		/// [JP] ローカルの変更を新しい Revision としてチームへ送り、ライブラリの内容を置き換える。
		Publish,

		/// [EN] Remove a shared asset from the library, Drive contents included, while the local files stay.
		/// [JP] 共有アセットを、Drive 上の中身も含めてライブラリから取り除く。ローカルのファイルは残す。
		Unshare,

		/// [EN] Stop following whatever this machine held at a path it has just deleted, while the library keeps it.
		/// [JP] この PC が今削除した位置にあったものを追いかけるのをやめる。ライブラリには残す。
		Forget,

		/// [EN] Follow a scene from now on, so the pieces other members publish in it are picked up by the next refresh.
		/// [JP] その Scene を以後追いかける。他のメンバーがその中で Publish した断片を、次の読み直しで拾うようにするため。
		OpenScene,
	};

	/**
	* [EN]
	* One queued piece of work, named by what it acts on rather than by
	* how it is carried out.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 待ち行列に入る作業1つ。どう実行するかではなく、何に対して行うかで
	* 表す。
	*/
	struct SharingRequest
	{
		/// [EN] What to do.
		/// [JP] 何をするか。
		SharingAction action_ = SharingAction::Refresh;

		/// [EN] Which shared asset it concerns; empty for a plain refresh.
		/// [JP] どの共有アセットに関するものか。単なる再読み込みでは空。
		String assetId_;

		/// [EN] Where the asset sits locally, for a first-time share, or the deleted file or folder, for a forget.
		/// [JP] 初めて共有する際の、ローカルでの位置。Forget では削除したファイルかフォルダの位置。
		String path_;

		/// [EN] The engine's 32-bit identifier and asset kind, for a first-time share.
		/// [JP] 初めて共有する際の、エンジンの32ビット識別子とアセットの種類。
		Uint32 runtimeId_ = 0;
		Int32 type_ = 0;
	};

	/**
	* [EN]
	* How one shared asset stands between this workspace and the library.
	* Only the two states the catalog cannot state by itself are kept here,
	* since the rest is already in the catalog.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有アセット1つが、このワークスペースとライブラリの間でどう立って
	* いるか。カタログだけでは言えない2つの状態のみを持つ。残りはカタログ
	* に既にあるため。
	*/
	struct SharedProgress
	{
		/// [EN] Which shared asset this describes.
		/// [JP] どの共有アセットについてのものか。
		String assetId_;

		/// [EN] Local files differ from what arrived, so this workspace holds work the team has not seen.
		/// [JP] ローカルのファイルが、届いた時点と違う。つまりチームがまだ見ていない作業を抱えている。
		Bool modified_ = false;

		/// [EN] Modified locally while the library also moved ahead, so publishing now replaces another member's newer work.
		/// [JP] ローカルで変更した上に、ライブラリも先へ進んでいる。今公開すると、他のメンバーの新しい作業を置き換えることになる。
		Bool conflicted_ = false;
	};

	/**
	* [EN]
	* What the Editor thread is allowed to look at: a copy of the shared
	* state taken while the worker was not writing to it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Editor のスレッドが見てよいもの。ワーカーが書き換えていない瞬間に
	* 取った、共有状態の写し。
	*/
	struct SharingSnapshot
	{
		/// [EN] Whether the last exchange with Google succeeded.
		/// [JP] 直近の Google とのやり取りが成功したかどうか。
		Bool online_ = false;

		/// [EN] What the worker is doing right now, for the Editor to show.
		/// [JP] ワーカーが今行っていること。Editor で表示するために使う。
		String operation_;

		/// [EN] The last failure, if any.
		/// [JP] 直近の失敗内容。
		String error_;

		/// [EN] The shared catalog as last read.
		/// [JP] 直近に読み取った共有カタログ。
		DynamicArray<SharedAsset> assets_;

		/// [EN] Only the assets that stand apart from the library; one that matches it is left out.
		/// [JP] ライブラリと食い違っているアセットのみ。一致しているものは載せない。
		DynamicArray<SharedProgress> progress_;

		/// [EN] Changes whenever the contents above change, so the Editor knows when to rebuild its view.
		/// [JP] 上の内容が変わるたびに変化する。Editor が表示を作り直す契機にする。
		Uint64 revision_ = 0;
	};

	/**
	* [EN]
	* The background half of asset sharing. It owns every connection to
	* Google, carries out queued work one item at a time, re-reads the
	* catalog on a timer, and leaves a snapshot behind for the Editor
	* thread to read.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アセット共有の裏側。Google との接続を全て所有し、待ち行列の作業を
	* 1つずつ実行し、定期的にカタログを読み直し、Editor のスレッドが
	* 読むための写しを残す。
	*/
	class SEEDCORE_API SharingWorker
	{
	public:
		/**
		* [EN]
		* Prepares the worker for a project, without connecting yet.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プロジェクト用にワーカーを用意する。この時点ではまだ接続しない。
		*/
		SharingWorker(const std::filesystem::path& projectRoot, const SharingConfig& config);

		/**
		* [EN]
		* Stops the thread.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スレッドを止める。
		*/
		~SharingWorker();

		/**
		* [EN]
		* Copy construction is disallowed, since one worker owns one thread.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー構築は禁止する。1つのワーカーが1つのスレッドを所有するため。
		*/
		SharingWorker(const SharingWorker&) = delete;

		/**
		* [EN]
		* Copy assignment is disallowed for the same reason as copy
		* construction.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー代入も、コピー構築と同じ理由で禁止する。
		*/
		SharingWorker& operator=(const SharingWorker&) = delete;

		/**
		* [EN]
		* Starts the background thread. Signing in happens on that thread,
		* so the Editor window never waits for a browser.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 裏のスレッドを開始する。ログインもそのスレッドで行うため、Editor
		* のウィンドウがブラウザを待つことはない。
		*/
		void Start();

		/**
		* [EN]
		* Stops the thread, letting it finish what it is doing first.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* スレッドを止める。実行中の作業は終わらせてから止める。
		*/
		void Stop();

		/**
		* [EN]
		* Hands one piece of work to the background thread and returns at
		* once.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 作業を1つ裏のスレッドへ渡し、すぐに戻る。
		*/
		void Enqueue(const SharingRequest& request);

		/**
		* [EN]
		* Returns a copy of the shared state for the Editor thread to read
		* without holding anything up.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 共有状態の写しを返す。Editor のスレッドが、何も待たせずに読める
		* ようにするため。
		*/
		SharingSnapshot Snapshot()const;

		/**
		* [EN]
		* Hands over the assets whose files this worker has replaced since
		* the last call, and forgets them. The Editor uses this to reload
		* what it already has open rather than keeping the old contents
		* until it is restarted.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前回の呼び出し以降にこのワーカーがファイルを差し替えたアセットを
		* 引き渡し、こちらからは忘れる。Editor はこれを使って、開いたままの
		* ものを読み直す。再起動まで古い中身を持ち続けないようにするため。
		*/
		void ConsumeChanged(DynamicArray<Uint32>& assets);

		/**
		* [EN]
		* Hands over the workspace paths of files taken in from the
		* library's Assets folder since the last call, and forgets them.
		* The Editor scans them so they get their identity; sharing them
		* is left to the member.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前回の呼び出し以降にライブラリの Assets フォルダから取り込んだ
		* ファイルの位置を引き渡し、こちらからは忘れる。Editor はそれらを
		* 走査して識別情報を与える。共有はメンバーに任せる。
		*/
		void ConsumeImported(DynamicArray<String>& paths);

	private:
		/**
		* [EN]
		* The background thread itself: signs in, then loops between
		* carrying out requests and re-reading the shared state until asked
		* to stop.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 裏のスレッド本体。ログインし、その後は停止を求められるまで、要求の
		* 実行と共有状態の読み直しを繰り返す。
		*/
		void Run();

		/**
		* [EN]
		* Carries out one request and records what happened.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 要求を1つ実行し、その結果を記録する。
		*/
		void Process(const SharingRequest& request);

		/**
		* [EN]
		* Brings this workspace up to the catalog just read: every asset it
		* already has that the library moved ahead on or that lost files, the
		* open scene's pieces, and whatever was dropped into the library's
		* Assets folder.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直前に読んだカタログまで、このワークスペースを引き上げる。対象は、
		* 既に持っているアセットのうちライブラリが先へ進んだもの・ファイルが
		* 欠けたもの、開いている Scene の断片、ライブラリの Assets フォルダへ
		* 置かれたもの。
		*/
		void Pull();

		/**
		* [EN]
		* Brings the local copy of an asset up to the catalog's revision,
		* downloading only the files whose contents differ.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットのローカルの写しを、カタログの Revision まで引き上げる。
		* 中身が異なるファイルだけをダウンロードする。
		*/
		Bool Get(const String& assetId);

		/**
		* [EN]
		* Replaces the local copy of an asset with the library's, .meta
		* included, even where the local files differ from what was last
		* recorded.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットのローカルの写しを、.meta も含めてライブラリのもので
		* 置き換える。ローカルのファイルが最後の記録と異なっていても置き換える。
		*/
		Bool Adopt(const String& assetId);

		/**
		* [EN]
		* Uploads whichever of an asset's files changed and records them as
		* the next revision.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットのファイルのうち変更されたものをアップロードし、次の
		* Revision として記録する。
		*/
		Bool Publish(const String& assetId);

		/**
		* [EN]
		* Shares a local asset for the first time, together with the .meta
		* and any files it reads from beside itself. When the library
		* already holds the same path, the library's copy is taken instead.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローカルのアセットを、隣の .meta と、アセットが隣から読み込む
		* ファイルと一緒に初めて共有する。ライブラリが既に同じ位置を持って
		* いる場合は、代わりにライブラリの写しを取る。
		*/
		Bool Register(const SharingRequest& request);

		/**
		* [EN]
		* Removes a shared asset from the library: its catalog entry goes,
		* and the Drive files holding its contents are moved to the trash
		* unless another asset still uses the same contents. The local files
		* stay where they are.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 共有アセットをライブラリから取り除く。カタログの項目を消し、中身を
		* 持つ Drive のファイルは、同じ中身を他のアセットが使っていない限り
		* ゴミ箱へ移す。ローカルのファイルはそのまま残す。
		*/
		Bool Unshare(const String& assetId);

		/**
		* [EN]
		* Stops following every shared asset whose files sat at or under the
		* given workspace path, which this machine has just deleted. The
		* library keeps them, and a later fetch brings them back only when
		* the member asks for them.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この PC が今削除した、指定のワークスペース内の位置（またはその下）に
		* ファイルがあった共有アセットを、追いかけるのをやめる。ライブラリには
		* 残り、メンバーが求めたときにだけ再び取得される。
		*/
		void Forget(const String& logicalPath);

		/**
		* [EN]
		* Keeps the library's Assets folder shaped like the workspace's
		* own, and takes in whatever has been dropped into it. Folders
		* are created where they are missing; files are downloaded to the
		* matching place and moved aside so they arrive only once.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ライブラリの Assets フォルダを、ワークスペース側と同じ形に保ち、
		* そこへ置かれたものを取り込む。足りないフォルダは作り、この PC が
		* まだ持っていない中身のファイルをダウンロードする。
		*/
		void Mirror(const std::filesystem::path& localFolder, const String& driveFolderId);

		/**
		* [EN]
		* Uploads one local file unless its contents are already stored,
		* and describes where they ended up.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローカルのファイル1つを、中身がまだ保存されていない場合に限り
		* アップロードし、その置き場所を返す。
		*/
		Bool Store(const String& logicalPath, SharedFile& stored);

		/**
		* [EN]
		* The workspace paths of the files an asset reads from beside itself,
		* which have to travel with it. Only a .gltf has any: its buffers and
		* images may live in separate files that its JSON names by URI.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットが隣から読み込むファイルの、ワークスペース内の位置。
		* アセットと一緒に運ぶ必要がある。持つのは .gltf だけで、その
		* バッファと画像は、JSON が URI で示す別ファイルに置かれることがある。
		*/
		DynamicArray<String> Companions(const String& logicalPath)const;

		/**
		* [EN]
		* Turns a workspace-relative path into a path on this machine.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ワークスペース内の位置を、この PC 上のパスへ変換する。
		*/
		std::filesystem::path Local(const String& logicalPath)const;

		/**
		* [EN]
		* Copies the current catalog and local progress into the snapshot the
		* Editor thread reads.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 現在のカタログとローカルの進み具合を、Editor のスレッドが読む写しへ写し取る。
		*/
		void Capture();

		/**
		* [EN]
		* Reads which revision of each asset this machine last got or
		* published, so a publish can state what it was built on.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この PC が各アセットのどの Revision を最後に取得・公開したかを
		* 読み込む。Publish 時に「何を元にしたか」を示すために要る。
		*/
		void LoadWorkspace();

		/**
		* [EN]
		* Writes that record back, so it survives the Editor being closed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* その記録を書き戻す。Editor を閉じても残るようにするため。
		*/
		void SaveWorkspace()const;

	private:
		/// [EN] Where the project sits, which every local path is resolved against.
		/// [JP] プロジェクトの位置。ローカルのパスはすべてここを起点に解決する。
		std::filesystem::path projectRoot_;

		/// [EN] The agreed settings this worker runs with.
		/// [JP] このワーカーが従う、取り決め済みの設定。
		SharingConfig config_;

		/// [EN] The connection layer, in the order they depend on each other.
		/// [JP] 通信の各層。互いに依存する順に並べている。
		HttpClient http_;
		GoogleAuth auth_;
		GoogleDocument document_;
		GoogleDrive drive_;
		SharedCatalog catalog_;

		/// [EN] The background thread and the flag that asks it to finish.
		/// [JP] 裏のスレッドと、終了を促すための印。
		std::thread thread_;
		std::atomic<Bool> running_ = false;

		/// [EN] What this machine holds for each shared asset it has ever fetched or published.
		/// [JP] この PC が、取得・公開したことのある各共有アセットについて保持している内容。
		std::unordered_map<String, WorkspaceRecord> workspace_;

		/// [EN] The scene the Editor currently has open, which is the only one whose pieces are followed.
		/// [JP] Editor が今開いている Scene。断片を追いかける対象はこの1つだけ。

		/// [EN] Following every scene would mean reading one document per scene on every latest fetch, for scenes nobody is looking at.
		/// [JP] 全ての Scene を追うと、誰も見ていない Scene の分まで、最新の取得のたびにドキュメントを1つずつ読むことになる。
		String openScene_;

		/// [EN] Which revision of each piece of that scene this machine last wrote out.
		/// [JP] その Scene の各断片について、この PC が最後に書き出した Revision。
		std::unordered_map<String, Uint64> sceneRevisions_;

		/// [EN] Work waiting to be carried out, oldest first.
		/// [JP] 実行を待っている作業。古いものから順に並ぶ。
		std::queue<SharingRequest> requests_;

		/// [EN] Guards the queue against the Editor thread adding while the worker takes.
		/// [JP] Editor のスレッドが追加している最中にワーカーが取り出すことを防ぐ。
		mutable std::mutex requestMutex_;

		/// [EN] The copy the Editor thread reads, and the lock that keeps it whole while it is read.
		/// [JP] Editor のスレッドが読む写しと、読んでいる間それを壊さないための錠。
		SharingSnapshot snapshot_;
		mutable std::mutex snapshotMutex_;

		/// [EN] Assets whose files were replaced and not yet handed to the Editor, guarded by the same lock as the snapshot.
		/// [JP] ファイルを差し替えたが、まだ Editor へ渡していないアセット。写しと同じ錠で守る。
		DynamicArray<Uint32> changed_;

		/// [EN] Workspace paths taken in from the library's Assets folder and not yet handed to the Editor.
		/// [JP] ライブラリの Assets フォルダから取り込み、まだ Editor へ渡していない位置。
		DynamicArray<String> imported_;
	};
}
