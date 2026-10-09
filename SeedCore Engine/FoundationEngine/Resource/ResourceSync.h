#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Resource/Asset/Asset.h>
#include <FoundationEngine/Resource/Sharing/SharingWorker.h>

namespace SeedCore
{
	class World;

	/**
	* [EN]
	* The Editor's window onto the shared asset library. It answers what
	* the interface needs to know - what the team has and how this copy
	* stands against it - and hands anything that takes a network round
	* trip to the background worker. Nothing here blocks a frame.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Editor から見た共有アセットライブラリの窓口。画面が知りたいこと
	* （チームが何を持っているか、手元の写しがそれに対してどうか）に
	* 答え、通信を伴うものは裏のワーカーへ渡す。ここで1フレームが止まる
	* ことはない。
	*/
	class SEEDCORE_API ResourceSync
	{
	public:
		/**
		* [EN]
		* Reads the project's sharing configuration and, when one is
		* present, starts the background worker.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* プロジェクトの共有設定を読み、設定があれば裏のワーカーを開始する。
		*/
		explicit ResourceSync(const std::filesystem::path& projectRoot);

		/**
		* [EN]
		* Stops the worker, which saves this machine's workspace record on
		* the way out.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ワーカーを止める。その過程で、この PC のワークスペースの記録が
		* 保存される。
		*/
		~ResourceSync();

		/**
		* [EN]
		* Copy construction is disallowed, since one Editor has one connection to the library.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー構築は禁止する。1つの Editor が持つライブラリへの接続は1つであるため。
		*/
		ResourceSync(const ResourceSync&) = delete;

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
		ResourceSync& operator=(const ResourceSync&) = delete;

		/**
		* [EN]
		* Takes the worker's latest state. Called once a frame; it copies
		* what the worker left behind and never waits for the network.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ワーカーの最新の状態を取り込む。毎フレーム呼ばれ、ワーカーが
		* 残した写しを取るだけで、通信を待つことはない。
		*/
		void Update();

		/**
		* [EN]
		* Hands over the assets the library has replaced on disk since the
		* last call, and forgets them. The Editor reloads each one so a
		* revision that arrives while it is running takes effect without a
		* restart.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前回の呼び出し以降にライブラリがディスク上で差し替えたアセットを
		* 引き渡し、こちらからは忘れる。Editor は各アセットを読み直し、実行中
		* に届いた Revision が再起動なしで反映されるようにする。
		*/
		void ConsumeChangedAsset(DynamicArray<Uint32>& assets);

		/**
		* [EN]
		* Hands over the workspace paths of files the library has taken in
		* from its Assets folder since the last call, and forgets them. The
		* Editor scans them so they get their identity; sharing them is
		* left to the member.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 前回の呼び出し以降にライブラリが Assets フォルダから取り込んだ
		* ファイルの位置を引き渡し、こちらからは忘れる。Editor はそれらを
		* 走査して識別情報を与える。共有はメンバーに任せる。
		*/
		void ConsumeImportedAsset(DynamicArray<String>& paths);

		/**
		* [EN]
		* Brings down every asset the open world refers to that the team
		* has but this machine does not: whatever an actor's asset fields
		* name, and the prefab each actor was made from. Called once a
		* frame; it acts only once after the member asks for the latest.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 開いている world が参照しているアセットのうち、チームは持っていて
		* この PC には無いものを取得する。対象は、Actor のアセット参照
		* フィールドが示すものと、各 Actor の元になった Prefab。毎フレーム
		* 呼ばれるが、動くのはメンバーが最新の取得を求めた後の1回だけ。
		*/
		void FetchReferenced(World& world);

		/**
		* [EN]
		* Whether this project is set up for sharing at all. Everything
		* else is inert while this is false, so a solo project behaves
		* exactly as it did before sharing existed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このプロジェクトが共有の設定を持っているかどうか。false の間は
		* 他の機能も働かないため、1人のプロジェクトは共有機能が無かった頃と
		* 同じように動く。
		*/
		Bool Configured()const;

		/**
		* [EN]
		* Whether the last exchange with the library succeeded.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直近のライブラリとのやり取りが成功したかどうか。
		*/
		Bool Online()const;

		/**
		* [EN]
		* A number that changes whenever the shared state does. Panels keep
		* the last value they saw and rebuild their view when it differs.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 共有状態が変わるたびに変化する番号。各パネルは最後に見た値を
		* 保持し、違っていれば表示を作り直す。
		*/
		Uint64 Revision()const;

		/**
		* [EN]
		* Appends the assets the team has but this machine does not, so the
		* content browser can show them before anything is downloaded.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* チームは持っているがこの PC には無いアセットを追加する。何も
		* ダウンロードしていない段階で、コンテンツブラウザに出せるように
		* するため。
		*/
		void Gather(DynamicArray<AssetRecord>& assets)const;

		/**
		* [EN]
		* Whether the asset is in the library but not yet on this machine.
		* Such an asset can be shown and fetched, but not dragged into a
		* scene.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* そのアセットがライブラリにあり、まだこの PC に無いかどうか。
		* 表示と取得はできるが、Scene へドラッグすることはできない。
		*/
		Bool RemoteOnly(Uint32 assetId)const;

		/**
		* [EN]
		* Whether this machine holds changes to the asset that the team has
		* not been sent yet, which is what makes a publish worth offering.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* そのアセットについて、まだチームへ送っていない変更をこの PC が
		* 抱えているかどうか。Publish を勧める根拠になる。
		*/
		Bool Modified(Uint32 assetId)const;

		/**
		* [EN]
		* Whether the asset changed here and in the library both. Publishing
		* now would replace the other member's newer copy, and getting would
		* discard the work here, so a member has to choose.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* そのアセットが、こちらとライブラリの両方で変わっているかどうか。
		* 今公開すれば他のメンバーの新しい写しを置き換え、取得すればここでの
		* 作業を捨てることになるため、メンバーが選ぶ必要がある状態。
		*/
		Bool Conflicted(Uint32 assetId)const;

		/**
		* [EN]
		* Whether a path holds shared content, in which case the Editor's
		* own rename and move must not touch it, since the library knows the
		* content by where it sits. Deleting is local only and stays allowed.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* その位置に共有された内容があるかどうか。ある場合、Editor 側の
		* リネーム・移動で触ってはならない。ライブラリは内容を位置で知って
		* いるため。削除はローカルだけの操作なので許す。
		*/
		Bool Managed(const std::filesystem::path& path)const;

		/**
		* [EN]
		* Whether the file at path can be shared through the library at
		* all. Source code is not: it is shared through git, and carries no
		* .meta for the library to identify it by.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* その位置のファイルを、そもそもライブラリで共有できるかどうか。
		* ソースコードはできない。git で共有するものであり、ライブラリが
		* 識別に使う .meta も持たないため。
		*/
		Bool Shareable(const std::filesystem::path& path)const;

		/**
		* [EN]
		* Asks for the library's copy of an asset to be brought down to
		* this machine.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ライブラリにあるアセットを、この PC へ取得するよう要求する。
		*/
		void RequestGet(Uint32 assetId);

		/**
		* [EN]
		* Asks for the library's copy of an asset, .meta included, to
		* replace this machine's even where local files differ. This is how
		* a member settles an asset whose identifier or contents disagree
		* with the team's.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ライブラリにあるアセットの写しで、.meta も含めてこの PC のものを
		* 置き換えるよう要求する。ローカルのファイルが異なっていても置き換える。
		* 識別子や中身がチームのものと食い違うアセットを、メンバーが解消する
		* 手段。
		*/
		void RequestAdopt(Uint32 assetId);

		/**
		* [EN]
		* Asks for local changes to be sent to the team as a new revision.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローカルの変更を、新しい Revision としてチームへ送るよう要求
		* する。
		*/
		void RequestPublish(Uint32 assetId);

		/**
		* [EN]
		* Asks for a local asset to be shared with the team for the first
		* time.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローカルのアセットを、初めてチームへ共有するよう要求する。
		*/
		void RequestRegister(const AssetRecord& asset);

		/**
		* [EN]
		* Asks for a shared asset to be removed from the library, its Drive
		* contents included. The local files stay where they are.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 共有アセットを、Drive 上の中身も含めてライブラリから取り除くよう
		* 要求する。ローカルのファイルはそのまま残る。
		*/
		void RequestUnshare(Uint32 assetId);

		/**
		* [EN]
		* Tells the library that this machine has deleted the file or folder
		* at path, so whatever shared assets sat there stop being followed.
		* The library keeps them; a later get brings them back on request.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この PC がその位置のファイルかフォルダを削除したことをライブラリへ
		* 伝え、そこにあった共有アセットを追いかけないようにする。ライブラリ
		* には残り、求めれば後から取得し直せる。
		*/
		void RequestForget(const std::filesystem::path& path);

		/**
		* [EN]
		* Asks for the shared state to be re-read now and for everything
		* the library moved ahead on to be brought down: assets this machine
		* already has, the open scene, and whatever the open world refers to
		* but this machine lacks. Nothing comes down except through this.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 今すぐ共有状態を読み直し、ライブラリが先へ進んだものを降ろすよう
		* 要求する。対象は、この PC が既に持っているアセット、開いている
		* Scene、開いている world が参照していてこの PC に無いもの。これ以外
		* の経路で何かが降りてくることはない。
		*/
		void RequestRefresh();

		/**
		* [EN]
		* Tells the library which scene the Editor now has open, so the
		* entities other members publish in it are picked up by the next
		* refresh. An empty path means no scene is open.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Editor が今開いている Scene をライブラリへ伝える。その Scene で
		* 他のメンバーが Publish した Entity を、次の読み直しで拾うように
		* するため。空のパスは、Scene を開いていないことを表す。
		*/
		void RequestOpenScene(const std::filesystem::path& path);

		/**
		* [EN]
		* The library's view of one asset - its revision, its files, who
		* published it - or nullptr when the team does not have it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ライブラリから見たアセット1つ分の情報。Revision、構成ファイル
		* など。チームが持っていなければ nullptr。
		*/
		const SharedAsset* GetAsset(Uint32 assetId)const;

		/**
		* [EN]
		* The library's view of the asset that sits at the given path, or
		* nullptr when the team does not have it.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* その位置にあるアセットについての、ライブラリから見た情報。チームが
		* 持っていなければ nullptr。
		*/
		const SharedAsset* GetAsset(const std::filesystem::path& path)const;

		/**
		* [EN]
		* What the library is doing right now, or empty when it is idle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ライブラリが今行っていること。何もしていなければ空。
		*/
		const String& Operation()const;

		/**
		* [EN]
		* The last failure, in a form that can be shown to the user.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直近の失敗内容。ユーザーへそのまま表示できる形で返す。
		*/
		const String& Error()const;

	private:
		/**
		* [EN]
		* Turns a path on this machine into the workspace-relative form the
		* library uses, or an empty string when it lies outside.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* この PC 上のパスを、ライブラリが使うワークスペース内の位置へ
		* 変換する。範囲外であれば空文字列を返す。
		*/
		String Logical(const std::filesystem::path& path)const;

	private:
		/// [EN] Where the project sits, which every local path is resolved against.
		/// [JP] プロジェクトの位置。ローカルのパスはすべてここを起点に解決する。
		std::filesystem::path projectRoot_;

		/// [EN] The team's settings, read once at startup.
		/// [JP] チームの設定。起動時に一度だけ読む。
		SharingConfig config_;

		/// [EN] Whether a configuration was found at all.
		/// [JP] 設定が見つかったかどうか。
		Bool configured_ = false;

		/// [EN] The background half; absent when this project does not share.
		/// [JP] 裏側の半分。このプロジェクトが共有しない場合は存在しない。
		ResourcePtr<SharingWorker> worker_;

		/// [EN] The worker's state as of the last Update, which everything here answers from.
		/// [JP] 直近の Update 時点におけるワーカーの状態。ここでの回答は全てこれに基づく。
		SharingSnapshot snapshot_;

		/// [EN] Extensions of source code, which is shared through git rather than through the library.
		/// [JP] ソースコードの拡張子。ライブラリではなく git で共有するもの。
		std::set<std::string_view> sourceExtensions_ =
		{
			".h", ".cpp", ".cs", ".hlsl", ".hlsli",
		};

		/// [EN] Whether the member asked for the latest and the open world's references have not been walked since.
		/// [JP] メンバーが最新の取得を求め、その後まだ開いている world の参照を辿っていないかどうか。
		Bool fetchReferencedRequested_ = false;
	};
}
