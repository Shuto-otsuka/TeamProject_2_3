#include <FoundationEngine/File/FileUtility.h>

namespace SeedCore
{
	/**
	* [EN]
	* Reads the entire file at filePath and returns its contents as a
	* std::string. Returns an empty string if the file can't be opened.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath のファイル全体を読み込み、その内容を std::string として
	* 返す。ファイルを開けなければ空文字列を返す。
	*/
	std::string FileUtility::LoadFileText(String filePath)
	{
		/// [EN] Opened in binary mode so the text comes back byte for byte, with CRLF line endings left as they are.
		/// [JP] バイナリモードで開くので、CRLF の改行も変換されず、テキストがバイト単位でそのまま返る。
		std::ifstream ifs(filePath.c_str(), std::ios::binary);

		/// [EN] A missing or unreadable file is reported as empty text rather than an error.
		/// [JP] 存在しない・読めないファイルは、エラーではなく空のテキストとして返す。
		if (!ifs)
		{
			return "";
		}

		/// [EN] Reads from the current position to the end of the stream in one go; the extra parentheses keep the first argument from being parsed as a function declaration.
		/// [JP] 現在位置からストリームの終わりまでを一度に読む。先頭の引数を余分な括弧で囲み、関数宣言として解釈されないようにしている。
		return std::string((std::istreambuf_iterator<Char>(ifs)), std::istreambuf_iterator<Char>());
	}

	/**
	* [EN]
	* Reads the entire file at filePath and returns its raw bytes.
	* Returns an empty array if the file can't be opened.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath のファイル全体を読み込み、その生バイト列を返す。
	* ファイルを開けなければ空の配列を返す。
	*/
	DynamicArray<Uint8> FileUtility::LoadFileBinary(String filePath)
	{
		/// [EN] std::ios::ate opens with the read position at the end, so tellg() right away gives the file size.
		/// [JP] std::ios::ate で読み取り位置を末尾に置いて開くので、すぐに tellg() を呼べばファイルサイズが分かる。
		std::ifstream ifs(filePath.c_str(), std::ios::binary | std::ios::ate);

		/// [EN] A missing or unreadable file is reported as an empty array rather than an error.
		/// [JP] 存在しない・読めないファイルは、エラーではなく空の配列として返す。
		if (!ifs)
		{
			return {};
		}

		/// [EN] The buffer is sized once from the file size, so the whole file is read with a single allocation.
		/// [JP] ファイルサイズからバッファを一度だけ確保するので、ファイル全体を1回の確保で読める。
		Size size = static_cast<Size>(ifs.tellg());
		DynamicArray<Uint8> buffer(size);

		/// [EN] Rewind to the start, then read every byte straight into the buffer.
		/// [JP] 先頭へ戻してから、全バイトを直接バッファへ読み込む。
		ifs.seekg(0, std::ios::beg);
		ifs.read(reinterpret_cast<Char*>(buffer.data()), size);
		return buffer;
	}
}