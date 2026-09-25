#include <FoundationEngine/File/FileDirectory.h>

namespace SeedCore
{
	/**
	* [EN]
	* Returns the directory containing the currently running
	* executable (no trailing separator), as a path.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 現在実行中の実行ファイルを含むディレクトリを
	* （末尾の区切り文字なしの）path で返す。
	*/
	std::filesystem::path FileDirectory::ExecutableDirectory()
	{
		/// [EN] nullptr as the module asks for the executable of the current process itself (not a DLL), as a full path including the file name. The wide version keeps non-ASCII folder names intact.
		/// [JP] モジュールに nullptr を渡すと、DLL ではなく現在のプロセスの実行ファイル自身のパスを、ファイル名込みのフルパスで得られる。ワイド文字版なので日本語などのフォルダ名も壊れない。
		Wchar buffer[MAX_PATH]{};
		GetModuleFileNameW(nullptr, buffer, MAX_PATH);

		/// [EN] parent_path() drops the file name, leaving the folder the executable lives in.
		/// [JP] parent_path() でファイル名を落とし、実行ファイルが置かれているフォルダだけを残す。
		return std::filesystem::path(buffer).parent_path();
	}
}