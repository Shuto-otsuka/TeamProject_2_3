#include <FoundationEngine/File/FileDialog.h>
#include <commdlg.h>

#pragma comment(lib, "comdlg32.lib")

namespace SeedCore
{
	namespace
	{
		/**
		* [EN]
		* Builds a Win32 OPENFILENAME-style filter string: "filterName\0filterExt\0\0".
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Win32のOPENFILENAME形式のフィルタ文字列
		* 「filterName\0filterExt\0\0」を構築する。
		*/
		std::wstring MakeFilter(const Wchar* filterName, const Wchar* filterExt)
		{
			/// [EN] The filter is a list of NUL-separated (display name, pattern) pairs, so each '\0' is pushed explicitly; += would stop at it.
			/// [JP] フィルタは NUL 区切りの（表示名, パターン）の組の並び。+= は '\0' で止まるので、'\0' は1つずつ明示的に足す。
			std::wstring filter;
			filter += filterName;
			filter.push_back(L'\0');
			filter += filterExt;
			filter.push_back(L'\0');

			/// [EN] One more NUL (a double NUL in total) marks the end of the whole list.
			/// [JP] もう1つ NUL を足し（合わせて NUL 2つ）、一覧全体の終わりを示す。
			filter.push_back(L'\0');
			return filter;
		}
	}

	/**
	* [EN]
	* Shows the Open File dialog filtered by filterName/filterExt
	* (e.g. "Scene Files"/"*.scene"), starting in initialDir. On
	* success, writes the chosen path to outPath and returns true;
	* returns false if the user cancels or the dialog fails.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filterName/filterExt（例: "Scene Files"/"*.scene"）でフィルタした
	* ファイルを開くダイアログを、initialDir から開始して表示する。
	* 成功時は選択されたパスを outPath に書き込み true を返す。
	* ユーザーがキャンセルしたか、ダイアログが失敗した場合は false。
	*/
	Bool FileDialog::OpenFile(std::filesystem::path& outPath, const std::filesystem::path& initialDir, const Wchar* filterName, const Wchar* filterExt)
	{
		/// [EN] Receives the chosen path; zero-filled so the dialog starts with an empty file name box.
		/// [JP] 選ばれたパスを受け取るバッファ。ゼロで埋め、ファイル名欄が空の状態でダイアログを開く。
		Wchar fileBuffer[MAX_PATH] = {};

		/// [EN] The dialog only keeps pointers into these strings, so they are kept alive as locals until it returns.
		/// [JP] ダイアログはこれらの文字列へのポインタしか持たないので、戻ってくるまでローカル変数として生かしておく。
		std::wstring filter = MakeFilter(filterName, filterExt);
		std::wstring initialDirStr = initialDir.wstring();

		/// [EN] Every field not set below must be zero, and lStructSize tells Windows which version of the struct this is.
		/// [JP] 以下で設定しない項目は全て 0 である必要があり、lStructSize で構造体のバージョンを Windows に伝える。
		OPENFILENAMEW ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);

		/// [EN] Owned by the active window, so the dialog is modal to it and stays in front of it.
		/// [JP] アクティブなウィンドウを親にするので、ダイアログはそれに対してモーダルになり、手前に表示される。
		ofn.hwndOwner = GetActiveWindow();
		ofn.lpstrFilter = filter.c_str();
		ofn.lpstrFile = fileBuffer;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = initialDirStr.c_str();

		/// [EN] Only an existing file in an existing folder can be chosen, and OFN_NOCHANGEDIR keeps the process's current directory unchanged, so relative paths elsewhere in the engine still resolve the same way.
		/// [JP] 選べるのは既存のフォルダにある既存のファイルだけ。OFN_NOCHANGEDIR でプロセスのカレントディレクトリを変えないので、エンジンの他の場所の相対パスは同じように解決され続ける。
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

		/// [EN] Returns zero both when the user cancels and when the dialog fails; outPath is left untouched in either case.
		/// [JP] ユーザーがキャンセルしたときもダイアログが失敗したときも 0 を返す。どちらの場合も outPath には触れない。
		if (!GetOpenFileNameW(&ofn))
		{
			return false;
		}

		/// [EN] fileBuffer now holds the full path of the chosen file.
		/// [JP] fileBuffer には選ばれたファイルのフルパスが入っている。
		outPath = fileBuffer;
		return true;
	}

	/**
	* [EN]
	* Shows the Save File dialog filtered by filterName/filterExt,
	* starting in initialDir with defaultExt appended if the user omits
	* one. On success, writes the chosen path to outPath and returns
	* true; returns false if the user cancels or the dialog fails.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filterName/filterExt でフィルタしたファイルを保存するダイアログを、
	* initialDir から開始して表示する。ユーザーが拡張子を省略した場合は
	* defaultExt が付与される。成功時は選択されたパスを outPath に
	* 書き込み true を返す。ユーザーがキャンセルしたか、ダイアログが
	* 失敗した場合は false。
	*/
	Bool FileDialog::SaveFile(std::filesystem::path& outPath, const std::filesystem::path& initialDir, const Wchar* filterName, const Wchar* filterExt, const Wchar* defaultExt, const Wchar* initialFileName)
	{
		/// [EN] Receives the chosen path; its initial contents are what the file name box shows when the dialog opens.
		/// [JP] 選ばれたパスを受け取るバッファ。開いた時点の中身が、そのままファイル名欄に表示される。
		Wchar fileBuffer[MAX_PATH] = {};

		/// [EN] Pre-fills the file name box with the suggested name; a name longer than the buffer is cut off instead of failing.
		/// [JP] 提案する名前をファイル名欄にあらかじめ入れる。バッファより長い名前は失敗にせず切り詰める。
		if (initialFileName != nullptr && initialFileName[0] != L'\0')
		{
			wcsncpy_s(fileBuffer, initialFileName, _TRUNCATE);
		}

		/// [EN] The dialog only keeps pointers into these strings, so they are kept alive as locals until it returns.
		/// [JP] ダイアログはこれらの文字列へのポインタしか持たないので、戻ってくるまでローカル変数として生かしておく。
		std::wstring filter = MakeFilter(filterName, filterExt);
		std::wstring initialDirStr = initialDir.wstring();

		/// [EN] Every field not set below must be zero, and lStructSize tells Windows which version of the struct this is.
		/// [JP] 以下で設定しない項目は全て 0 である必要があり、lStructSize で構造体のバージョンを Windows に伝える。
		OPENFILENAMEW ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);

		/// [EN] Owned by the active window, so the dialog is modal to it and stays in front of it.
		/// [JP] アクティブなウィンドウを親にするので、ダイアログはそれに対してモーダルになり、手前に表示される。
		ofn.hwndOwner = GetActiveWindow();
		ofn.lpstrFilter = filter.c_str();
		ofn.lpstrFile = fileBuffer;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = initialDirStr.c_str();

		/// [EN] Appended when the typed name has no extension (given without the dot, e.g. L"scene").
		/// [JP] 入力された名前に拡張子が無いときに付け足される（ドット無しで渡す。例: L"scene"）。
		ofn.lpstrDefExt = defaultExt;

		/// [EN] Asks before overwriting an existing file and requires the folder to exist; OFN_NOCHANGEDIR keeps the process's current directory unchanged, so relative paths elsewhere still resolve the same way.
		/// [JP] 既存ファイルを上書きする前に確認し、フォルダが存在することを求める。OFN_NOCHANGEDIR でカレントディレクトリを変えないので、他の場所の相対パスは同じように解決され続ける。
		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

		/// [EN] Returns zero both when the user cancels and when the dialog fails; outPath is left untouched in either case.
		/// [JP] ユーザーがキャンセルしたときもダイアログが失敗したときも 0 を返す。どちらの場合も outPath には触れない。
		if (!GetSaveFileNameW(&ofn))
		{
			return false;
		}

		/// [EN] fileBuffer now holds the full path to save to, with defaultExt already appended when needed.
		/// [JP] fileBuffer には保存先のフルパスが入っている。必要なら defaultExt も既に付いている。
		outPath = fileBuffer;
		return true;
	}
}
