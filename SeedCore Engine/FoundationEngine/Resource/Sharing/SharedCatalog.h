#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Resource/Sharing/GoogleDocument.h>

namespace SeedCore
{
	/**
	* [EN]
	* One file belonging to a shared asset: where it sits in the workspace,
	* what its contents hash to, and which stored blob holds them. An asset
	* is usually its own file plus the .meta beside it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有アセットに属するファイル1つ。ワークスペース上の位置、中身の
	* ハッシュ、そしてその中身を保持している blob を表す。アセットは通常、
	* 本体と隣の .meta の2つで構成される。
	*/
	struct SharedFile
	{
		/// [EN] Where the file belongs inside the workspace, written with forward slashes.
		/// [JP] ワークスペース内での位置。区切りは斜線で書く。
		String path_;

		/// [EN] SHA-256 of the contents, which is also the name the blob is stored under.
		/// [JP] 中身の SHA-256。blob を保存するときの名前でもある。
		String hash_;

		/// [EN] Identifier of the Drive file holding those contents.
		/// [JP] その中身を保持している Drive のファイルの識別子。
		String driveId_;

		/// [EN] Size in bytes, checked after a download to catch a truncated transfer.
		/// [JP] バイト数。ダウンロード後に照合して、途中で切れた転送を検出する。
		Uint64 size_ = 0;
	};

	/**
	* [EN]
	* One asset as the team knows it: its identity, its current revision,
	* and the files that make it up. This is what an Editor compares its
	* local copy against to decide whether it is up to date.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* チームから見たアセット1つ。識別情報、現在の Revision、構成する
	* ファイルを持つ。Editor は手元の写しをこれと比べて、最新かどうかを
	* 判断する。
	*/
	struct SharedAsset
	{
		/// [EN] Identifier within the shared library, issued once when the asset is first published.
		/// [JP] 共有ライブラリ内での識別子。最初に Publish した時に一度だけ発行する。
		String id_;

		/// [EN] Workspace path of the asset itself, as opposed to its sidecar files.
		/// [JP] アセット本体のワークスペース上の位置。付随するファイルとは区別する。
		String path_;

		/// [EN] The 32-bit identifier the engine uses locally, taken from the asset's .meta.
		/// [JP] エンジンがローカルで使う32ビットの識別子。アセットの .meta から取る。
		Uint32 runtimeId_ = 0;

		/// [EN] Which kind of asset this is, matching the engine's own AssetType values.
		/// [JP] アセットの種類。エンジン側の AssetType の値と一致する。
		Int32 type_ = 0;

		/// [EN] Increases by one on every publish; a publish from an older number is refused.
		/// [JP] Publish のたびに1つ増える。これより古い番号からの Publish は拒否される。
		Uint64 revision_ = 0;

		/// [EN] Every file the asset is made of, the asset itself included.
		/// [JP] アセットを構成する全ファイル。本体もここに含む。
		DynamicArray<SharedFile> files_;

		/// [EN] Other shared assets this one needs, so getting it can pull them along.
		/// [JP] このアセットが必要とする他の共有アセット。取得時に一緒に引いてくるために使う。
		DynamicArray<String> dependencies_;

		/// [EN] Whether entity-level editing applies, which is true for scenes and false for everything else.
		/// [JP] Entity 単位の編集を行うかどうか。Scene では true、それ以外では false。
		Bool scene_ = false;

		/// [EN] For a scene, the document holding its per-piece revisions; empty for every other kind of asset.
		/// [JP] Scene の場合、断片ごとの Revision を持つドキュメント。それ以外の種類では空。
		String sceneDocumentId_;
	};

	/**
	* [EN]
	* The shared catalog, held in one Google document. It is what makes a
	* remote asset visible in the Editor before any of its content has been
	* downloaded, and its revision numbers are what stop an older publish
	* from overwriting a newer one.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有カタログ。1つの Google ドキュメントに置かれる。中身を1つも
	* ダウンロードしていない段階で、リモートのアセットを Editor に見せる
	* 役割を持つ。ここの Revision 番号が、古い Publish が新しいものを
	* 上書きすることを防いでいる。
	*/
	class SEEDCORE_API SharedCatalog
	{
	public:
		/**
		* [EN]
		* Takes the document this catalog lives in.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このカタログが置かれているドキュメントを受け取る。
		*/
		SharedCatalog(GoogleDocument& document, const String& documentId);

		/**
		* [EN]
		* Drops the in-memory copy; the catalog itself lives in the
		* document and outlives any Editor.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* メモリ上の写しを捨てるだけ。カタログ自体はドキュメントにあり、
		* どの Editor よりも長く残る。
		*/
		~SharedCatalog();

		/**
		* [EN]
		* Copy construction is disallowed, since the in-memory copy tracks one document at one point in time.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー構築は禁止する。メモリ上の写しは、ある時点のドキュメント1つを表すものであるため。
		*/
		SharedCatalog(const SharedCatalog&) = delete;

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
		SharedCatalog& operator=(const SharedCatalog&) = delete;

		/**
		* [EN]
		* Re-reads the catalog so the Editor sees what the team currently
		* has and at which revisions.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* カタログを読み直し、チームが今何を持っていて、それぞれどの
		* Revision なのかを Editor へ反映する。
		*/
		Bool Refresh();

		/**
		* [EN]
		* Every asset in the catalog as of the last read.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 直近の読み取り時点でカタログにある全アセット。
		*/
		const DynamicArray<SharedAsset>& Assets()const;

		/**
		* [EN]
		* Looks an asset up by its shared identifier. Returns nullptr when
		* the catalog holds no such asset.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 共有識別子でアセットを引く。該当が無ければ nullptr を返す。
		*/
		const SharedAsset* Find(const String& assetId)const;

		/**
		* [EN]
		* Looks an asset up by the 32-bit identifier the engine uses
		* locally. Returns nullptr when the catalog holds no such asset.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* エンジンがローカルで使う32ビットの識別子でアセットを引く。該当が
		* 無ければ nullptr を返す。
		*/
		const SharedAsset* Find(Uint32 runtimeId)const;

		/**
		* [EN]
		* Looks an asset up by its workspace path. Returns nullptr when the
		* catalog holds no such asset.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ワークスペース上の位置でアセットを引く。該当が無ければ nullptr を
		* 返す。
		*/
		const SharedAsset* FindPath(const String& path)const;

		/**
		* [EN]
		* Adds a local asset to the catalog as revision 1. Fails when its
		* path or its 32-bit identifier is already taken, which is how two
		* members creating the same thing separately is caught.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ローカルのアセットを Revision 1 としてカタログへ加える。位置か
		* 32ビット識別子が既に使われていれば失敗する。2人が別々に同じものを
		* 作ってしまった場合は、これで検出できる。
		*/
		Bool Register(const SharedAsset& asset);

		/**
		* [EN]
		* Records new contents for an asset and moves it to the next
		* revision, replacing whatever the library held: the last publish
		* wins. revision receives the revision that was written.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットの新しい中身を記録し、次の Revision へ進める。ライブラリの
		* 内容は置き換えられ、最後に Publish したものが優先される。revision
		* には書き込んだ Revision が入る。
		*/
		Bool Publish(const String& assetId, const DynamicArray<SharedFile>& files, const DynamicArray<String>& dependencies, Uint64& revision);

		/**
		* [EN]
		* Removes an asset's entry from the catalog altogether. Members who
		* already hold a copy keep it as an ordinary local file; the library
		* simply stops knowing the asset.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* アセットの項目をカタログから丸ごと取り除く。既に写しを持っている
		* メンバーの手元には、普通のローカルファイルとして残る。ライブラリが
		* そのアセットを知らなくなるだけ。
		*/
		Bool Remove(const String& assetId);

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
		* Reads the catalog, applies edit to it, and writes it back,
		* retrying from the fresh contents whenever another member wrote
		* first.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* カタログを読み、edit を適用して書き戻す。他のメンバーが先に
		* 書いていた場合は、新しい内容から読み直してやり直す。
		*/
		Bool Modify(const std::function<Bool(nlohmann::json&)>& edit);

		/**
		* [EN]
		* Parses the document text into the in-memory asset list.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ドキュメントの本文を解釈して、メモリ上のアセット一覧にする。
		*/
		void Adopt(const nlohmann::json& catalog);

	private:
		/// [EN] Reads and rewrites the document this catalog lives in.
		/// [JP] このカタログが置かれているドキュメントの読み書きを行う。
		GoogleDocument& document_;

		/// [EN] Which document that is.
		/// [JP] そのドキュメントがどれかを示す識別子。
		String documentId_;

		/// [EN] The catalog as of the last read, in the order the document lists it.
		/// [JP] 直近の読み取り時点のカタログ。ドキュメントに並んでいる順のまま。
		DynamicArray<SharedAsset> assets_;

		/// [EN] Last failure description; empty while everything is working.
		/// [JP] 直近の失敗の説明。問題なく動いている間は空。
		String error_;
	};
}
