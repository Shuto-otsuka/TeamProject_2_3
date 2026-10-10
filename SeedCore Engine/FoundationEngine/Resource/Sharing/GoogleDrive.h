#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/Resource/Sharing/GoogleAuth.h>
#include <FoundationEngine/Resource/Sharing/HttpClient.h>

namespace SeedCore
{
	/**
	* [EN]
	* One child of a Drive folder, as a listing reports it.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Drive のフォルダの子1つ。一覧で得られる内容を表す。
	*/
	struct DriveEntry
	{
		/// [EN] Identifier of the file or folder.
		/// [JP] そのファイルまたはフォルダの識別子。
		String id_;

		/// [EN] Name as the member who put it there typed it.
		/// [JP] 置いた人が付けたままの名前。
		String name_;

		/// [EN] Whether this child is itself a folder, which a walk has to descend into.
		/// [JP] その子自身がフォルダかどうか。走査ではここへ降りていく。
		Bool folder_ = false;

		/// [EN] SHA-256 of the contents as Drive computed it, which tells an already-taken-in file from a changed one without downloading.
		/// [JP] Drive 側が計算した中身の SHA-256。ダウンロードせずに、取り込み済みのファイルと差し替えられたファイルを見分けられる。

		/// [EN] Empty for a folder, and for the rare file Drive has not hashed; the size then stands in for it.
		/// [JP] フォルダと、まれに Drive がハッシュを持たないファイルでは空になる。その場合はサイズで代用する。
		String hash_;

		/// [EN] Size in bytes as Drive reports it.
		/// [JP] Drive が示すバイト数。
		Uint64 size_ = 0;

		/// [EN] When it last changed, in UTC as RFC 3339 text such as "2026-10-09T12:34:56.789Z", which orders correctly as plain text.
		/// [JP] 最後に変わった時刻。"2026-10-09T12:34:56.789Z" のような UTC の RFC 3339 形式で、文字列のまま比べても順序が正しい。
		String modified_;
	};

	/**
	* [EN]
	* The shared library's file storage. Asset contents live in a Drive
	* folder as one file per unique content hash: a file is written once
	* and never modified, so a download can never race a change, and two
	* revisions that share content share one file.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 共有ライブラリのファイル置き場。アセットの中身は、内容のハッシュ
	* ごとに1ファイルとして Drive のフォルダに置く。一度書いたファイルは
	* 変更しないので、ダウンロード中に中身が変わることがなく、内容が同じ
	* Revision は1つのファイルを共有する。
	*/
	class SEEDCORE_API GoogleDrive
	{
	public:
		/**
		* [EN]
		* Takes the transport and the sign-in that every request depends
		* on.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* すべてのリクエストが依存する、通信手段とログイン情報を受け取る。
		*/
		GoogleDrive(HttpClient& http, GoogleAuth& auth);

		/**
		* [EN]
		* Nothing to undo: every transfer finishes before its call returns.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 後始末は要らない。どの転送も、呼び出しが戻る前に終わっているため。
		*/
		~GoogleDrive();

		/**
		* [EN]
		* Copy construction is disallowed, since the last error belongs to
		* the one caller that made the request.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* コピー構築は禁止する。直近のエラーは、そのリクエストを出した
		* 1つの呼び出し元に属するものであるため。
		*/
		GoogleDrive(const GoogleDrive&) = delete;

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
		GoogleDrive& operator=(const GoogleDrive&) = delete;

		/**
		* [EN]
		* Returns the identifier of the child of parentId named name, or an
		* empty string when the folder holds no such child.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* parentId の中にある name という名前の子の識別子を返す。該当する
		* 子が無ければ空文字列を返す。
		*/
		String Find(const String& name, const String& parentId);

		/**
		* [EN]
		* Lists what sits directly inside parentId. Used to walk the library's
		* Assets folder and to sweep its blobs folder.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* parentId の直下にあるものを一覧する。ライブラリの Assets フォルダを
		* 辿るのと、blobs フォルダを掃除するのに使う。
		*/
		Bool List(const String& parentId, DynamicArray<DriveEntry>& entries);

		/**
		* [EN]
		* Creates a folder named name inside parentId and returns its
		* identifier.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* parentId の中に name という名前のフォルダを作り、その識別子を
		* 返す。
		*/
		String Folder(const String& name, const String& parentId);

		/**
		* [EN]
		* Uploads source into parentId under name and returns the new
		* file's identifier. The transfer is resumable, so a connection
		* dropping midway does not mean starting over.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* source を parentId の中へ name という名前でアップロードし、新しい
		* ファイルの識別子を返す。転送は再開可能なので、途中で接続が切れても
		* 最初からやり直しにはならない。
		*/
		String Upload(const std::filesystem::path& source, const String& name, const String& parentId);

		/**
		* [EN]
		* Downloads the file named by fileId to destination, writing it
		* straight to disk rather than holding it in memory.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* fileId のファイルを destination へダウンロードする。メモリに
		* 載せず、ディスクへ直接書き出す。
		*/
		Bool Fetch(const String& fileId, const std::filesystem::path& destination);

		/**
		* [EN]
		* Moves the file named by fileId to the owner's trash. Used only to
		* clean up content no revision refers to any more.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* fileId のファイルを、所有者のゴミ箱へ移す。どの Revision からも
		* 参照されなくなった中身の掃除にだけ使う。
		*/
		Bool Trash(const String& fileId);

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
		* Sends one authenticated request to the Drive API and parses its
		* JSON answer, recording any failure.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 認証付きのリクエストを Drive API へ1回送り、JSON の応答を解釈
		* する。失敗した場合はその内容も記録する。
		*/
		Bool Send(const String& method, const String& url, const nlohmann::json& request, nlohmann::json& result);

	private:
		/// [EN] Transport shared with the rest of the sharing layer.
		/// [JP] 共有機能の他の部分と共用する通信手段。
		HttpClient& http_;

		/// [EN] Supplies the access token every request carries.
		/// [JP] 各リクエストに付けるアクセストークンの供給元。
		GoogleAuth& auth_;

		/// [EN] Last failure description; empty while everything is working.
		/// [JP] 直近の失敗の説明。問題なく動いている間は空。
		String error_;
	};
}
