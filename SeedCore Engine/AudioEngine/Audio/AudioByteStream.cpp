#include <External/CRI/Include/cri_adx2le.h>
#include <AudioEngine/Audio/AudioByteStream.h>
#include <FoundationEngine/Serialization/Encryption/Aes256.h>
#include <FoundationEngine/Serialization/Encryption/Sha256.h>

namespace SeedCore
{
	/**
	* [JP]
	* 音のライブラリからファイルを読むときに呼ばれる関数をまとめたもの。
	* エンジンが暗号化した .audio の中の AWB も、ここで復号しながら読む。
	*
	* @brief ファイルを読むための関数の集まり
	*
	* [EN]
	* The functions the sound library calls when it reads a file.
	* AWB data inside an engine-encrypted .audio is read here, decrypting as it goes.
	*
	* @brief The functions used to read files
	*/
	class AudioByteStream::Impl final
	{
	public:
		/**
		* [JP]
		* どのファイルでも、この読み方を使うように答える。
		*
		* @brief  読み方を選ぶ
		* @param  path        読もうとしているファイル
		* @param  deviceID    使う装置が入る
		* @param  ioInterface 使う読み方が入る
		* @return いつも成功
		*
		* [EN]
		* Answers that every file should be read this way.
		*
		* @brief  Picks the reader
		* @param  path        The file about to be read
		* @param  deviceID    Gets the device to use
		* @param  ioInterface Gets the reader to use
		* @return Always succeeds
		*/
		static CriError CRIAPI SelectIo(const CriChar8* path, CriFsDeviceId* deviceID, CriFsIoInterfacePtr* ioInterface);

		/**
		* [JP]
		* ファイルがあるかを調べる。
		*
		* @brief  ファイルがあるか調べる
		* @param  path   調べるファイル
		* @param  result あれば CRI_TRUE が入る
		* @return 調べられたかどうか
		*
		* [EN]
		* Checks whether a file exists.
		*
		* @brief  Checks if a file exists
		* @param  path   The file to check
		* @param  result Gets CRI_TRUE if it exists
		* @return Whether the check worked
		*/
		static CriFsIoError CRIAPI Exists(const CriChar8* path, CriBool* result);

		/**
		* [JP]
		* ファイルを開く。暗号化されたものは、中の AWB の場所を調べておく。
		*
		* @brief  ファイルを開く
		* @param  path       開くファイル
		* @param  mode       開き方
		* @param  access     読むか書くか
		* @param  fileHandle 開いたファイルが入る
		* @return 開けたかどうか
		* @note   読むことしかできない
		*
		* [EN]
		* Opens a file. For an encrypted one, it looks up where the AWB inside is.
		*
		* @brief  Opens a file
		* @param  path       The file to open
		* @param  mode       How to open it
		* @param  access     Read or write
		* @param  fileHandle Gets the opened file
		* @return Whether it opened
		* @note   It can only read
		*/
		static CriFsIoError CRIAPI Open(const CriChar8* path, CriFsFileMode mode, CriFsFileAccess access, CriFsFileHn* fileHandle);

		/**
		* [JP]
		* 開いたファイルを閉じる。
		*
		* @brief  ファイルを閉じる
		* @param  fileHandle 閉じるファイル
		* @return いつも成功
		*
		* [EN]
		* Closes an opened file.
		*
		* @brief  Closes a file
		* @param  fileHandle The file to close
		* @return Always succeeds
		*/
		static CriFsIoError CRIAPI Close(CriFsFileHn fileHandle);

		/**
		* [JP]
		* ファイルの大きさ（復号したあとの大きさ）を返す。
		*
		* @brief  ファイルの大きさを返す
		* @param  fileHandle 調べるファイル
		* @param  fileSize   大きさが入る
		* @return 調べられたかどうか
		*
		* [EN]
		* Returns the file size (the size after decrypting).
		*
		* @brief  Returns the file size
		* @param  fileHandle The file to check
		* @param  fileSize   Gets the size
		* @return Whether it worked
		*/
		static CriFsIoError CRIAPI FileSize(CriFsFileHn fileHandle, CriSint64* fileSize);

		/**
		* [JP]
		* ファイルの一部を読む。暗号化されていれば、その部分だけ復号する。
		*
		* @brief  ファイルの一部を読む
		* @param  fileHandle 読むファイル
		* @param  offset     読み始める場所
		* @param  readSize   読みたい大きさ
		* @param  buffer     読んだデータが入る
		* @param  bufferSize buffer の大きさ
		* @return 読めたかどうか
		*
		* [EN]
		* Reads part of a file, decrypting only that part if it is encrypted.
		*
		* @brief  Reads part of a file
		* @param  fileHandle The file to read
		* @param  offset     Where to start reading
		* @param  readSize   How much to read
		* @param  buffer     Gets the data read
		* @param  bufferSize The size of buffer
		* @return Whether it read
		*/
		static CriFsIoError CRIAPI Read(CriFsFileHn fileHandle, CriSint64 offset, CriSint64 readSize, void* buffer, CriSint64 bufferSize);

		/**
		* [JP]
		* 読み込みが終わったかを答える。読み込みはすぐ終わるので、いつも終わっている。
		*
		* @brief  読み込みが終わったか答える
		* @param  fileHandle 調べるファイル
		* @param  result     終わっていれば CRI_TRUE が入る
		* @return 調べられたかどうか
		*
		* [EN]
		* Answers whether reading has finished; reading is instant, so it always has.
		*
		* @brief  Answers if reading is done
		* @param  fileHandle The file to check
		* @param  result     Gets CRI_TRUE when done
		* @return Whether the check worked
		*/
		static CriFsIoError CRIAPI Complete(CriFsFileHn fileHandle, CriBool* result);

		/**
		* [JP]
		* 最後に読めた大きさを返す。
		*
		* @brief  読めた大きさを返す
		* @param  fileHandle 調べるファイル
		* @param  readSize   読めた大きさが入る
		* @return 調べられたかどうか
		*
		* [EN]
		* Returns how much was read last time.
		*
		* @brief  Returns the amount read
		* @param  fileHandle The file to check
		* @param  readSize   Gets the amount read
		* @return Whether it worked
		*/
		static CriFsIoError CRIAPI ReadSize(CriFsFileHn fileHandle, CriSint64* readSize);

	private:
		/**
		* [JP]
		* 開いているファイル1つ分の情報。
		*
		* @brief 開いているファイルの情報
		*
		* [EN]
		* What we keep about one opened file.
		*
		* @brief Info about an opened file
		*/
		struct File
		{
			/// [JP] Windows のファイルの目印
			/// [EN] The Windows file handle
			HANDLE handle_ = INVALID_HANDLE_VALUE;

			/// [JP] 暗号化されているか
			/// [EN] Whether it is encrypted
			Bool encrypted_ = false;

			/// [JP] 復号に使う最初の値
			/// [EN] The starting value used to decrypt
			Byte iv_[16]{};

			/// [JP] 暗号化された中身がファイルのどこから始まるか
			/// [EN] Where the encrypted data starts in the file
			Uint64 ciphertextOffset_ = 0;

			/// [JP] 暗号化された中身の大きさ
			/// [EN] The size of the encrypted data
			Uint64 ciphertextSize_ = 0;

			/// [JP] 復号したあとの大きさ
			/// [EN] The size after decrypting
			Uint64 plainSize_ = 0;

			/// [JP] 最後に読めた大きさ
			/// [EN] How much was read last time
			Uint64 readSize_ = 0;
		};

		/// [JP] 音のライブラリに渡す、読み方の一覧
		/// [EN] The list of read functions given to the sound library
		static CriFsIoInterface ioInterface_;
	};

	/// [JP] 使う関数だけを入れ、使わないところは空にしておく
	/// [EN] Fill in only the functions we use and leave the rest empty
	CriFsIoInterface AudioByteStream::Impl::ioInterface_ =
	{
		&AudioByteStream::Impl::Exists,

		nullptr,
		nullptr,

		&AudioByteStream::Impl::Open,
		&AudioByteStream::Impl::Close,
		&AudioByteStream::Impl::FileSize,
		&AudioByteStream::Impl::Read,
		&AudioByteStream::Impl::Complete,

		nullptr,

		&AudioByteStream::Impl::ReadSize,

		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,

		nullptr,
		nullptr,
		nullptr,
	};

	/**
	* [JP]
	* どのファイルでも、この読み方を使うように答える。
	*
	* @brief  読み方を選ぶ
	* @param  path        読もうとしているファイル
	* @param  deviceID    使う装置が入る
	* @param  ioInterface 使う読み方が入る
	* @return いつも成功
	*
	* [EN]
	* Answers that every file should be read this way.
	*
	* @brief  Picks the reader
	* @param  path        The file about to be read
	* @param  deviceID    Gets the device to use
	* @param  ioInterface Gets the reader to use
	* @return Always succeeds
	*/
	CriError CRIAPI AudioByteStream::Impl::SelectIo(const CriChar8* path, CriFsDeviceId* deviceID, CriFsIoInterfacePtr* ioInterface)
	{
		if (deviceID)
		{
			*deviceID = CRIFS_DEFAULT_DEVICE;
		}

		if (ioInterface)
		{
			*ioInterface = &ioInterface_;
		}

		return CRIERR_OK;
	}

	/**
	* [JP]
	* ファイルがあるかを調べる。
	*
	* @brief  ファイルがあるか調べる
	* @param  path   調べるファイル
	* @param  result あれば CRI_TRUE が入る
	* @return 調べられたかどうか
	*
	* [EN]
	* Checks whether a file exists.
	*
	* @brief  Checks if a file exists
	* @param  path   The file to check
	* @param  result Gets CRI_TRUE if it exists
	* @return Whether the check worked
	*/
	CriFsIoError CRIAPI AudioByteStream::Impl::Exists(const CriChar8* path, CriBool* result)
	{
		if (!result)
		{
			return CRIFS_IO_ERROR_NG;
		}

		*result = CRI_FALSE;

		if (!path)
		{
			return CRIFS_IO_ERROR_OK;
		}

		/// [JP] 暗号化のしるしがついていたら外して、本当のファイルの場所にする
		/// [EN] Strip the encrypted marker to get the real file path
		std::string_view pathView(path);
		if (pathView.starts_with("seedcore_audio://"))
		{
			pathView.remove_prefix(std::strlen("seedcore_audio://"));
		}

		String filePath(pathView);
		*result = std::filesystem::exists(filePath.c_str()) ? CRI_TRUE : CRI_FALSE;

		return CRIFS_IO_ERROR_OK;
	}

	/**
	* [JP]
	* ファイルを開く。暗号化されたものは、中の AWB の場所を調べておく。
	*
	* @brief  ファイルを開く
	* @param  path       開くファイル
	* @param  mode       開き方
	* @param  access     読むか書くか
	* @param  fileHandle 開いたファイルが入る
	* @return 開けたかどうか
	* @note   読むことしかできない
	*
	* [EN]
	* Opens a file. For an encrypted one, it looks up where the AWB inside is.
	*
	* @brief  Opens a file
	* @param  path       The file to open
	* @param  mode       How to open it
	* @param  access     Read or write
	* @param  fileHandle Gets the opened file
	* @return Whether it opened
	* @note   It can only read
	*/
	CriFsIoError CRIAPI AudioByteStream::Impl::Open(const CriChar8* path, CriFsFileMode mode, CriFsFileAccess access, CriFsFileHn* fileHandle)
	{
		if (!fileHandle)
		{
			return CRIFS_IO_ERROR_NG;
		}

		*fileHandle = nullptr;

		/// [JP] 書き込みには対応していないので、開かずに終わる
		/// [EN] Writing is not supported, so finish without opening
		if (!path || access == CRIFS_FILE_ACCESS_WRITE || access == CRIFS_FILE_ACCESS_READ_WRITE)
		{
			return CRIFS_IO_ERROR_OK;
		}

		std::string_view pathView(path);
		Bool encrypted = pathView.starts_with("seedcore_audio://");
		if (encrypted)
		{
			pathView.remove_prefix(std::strlen("seedcore_audio://"));
		}

		String filePath(pathView);
		HANDLE handle = CreateFileW(filePath.w_str().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
		if (handle == INVALID_HANDLE_VALUE)
		{
			return CRIFS_IO_ERROR_OK;
		}

		/// [JP] 暗号化されていないファイルなら、大きさを読んでそのまま使う
		/// [EN] For a plain file, read its size and use it as it is
		File* file = new File();
		file->handle_ = handle;
		file->encrypted_ = encrypted;

		if (!encrypted)
		{
			LARGE_INTEGER fileSize{};
			GetFileSizeEx(handle, &fileSize);
			file->plainSize_ = static_cast<Uint64>(fileSize.QuadPart);

			*fileHandle = file;
			return CRIFS_IO_ERROR_OK;
		}

		/// [JP] 見出しを読んで、エンジンのファイルかを確かめる
		/// [EN] Read the header and check it is an engine file
		Byte header[24]{};
		DWORD readBytes = 0;
		OVERLAPPED headerOverlapped{};
		if (!ReadFile(handle, header, 24, &readBytes, &headerOverlapped) || readBytes != 24 || std::memcmp(header, "SCAUDIO\0", 8) != 0)
		{
			CloseHandle(handle);
			delete file;
			return CRIFS_IO_ERROR_NG_INVALID_DATA;
		}

		Uint32 blobCount = 0;
		std::memcpy(&blobCount, header + 16, 4);

		/// [JP] 中身の一覧から AWB の場所と大きさを探す
		/// [EN] Look through the parts list for where the AWB is and how big it is
		Uint64 blobOffset = 0;
		Uint64 blobEncryptedSize = 0;
		Uint64 blobPlainSize = 0;
		for (Uint32 blobIndex = 0; blobIndex < blobCount && blobIndex < 2; ++blobIndex)
		{
			Byte entry[32]{};
			OVERLAPPED entryOverlapped{};
			entryOverlapped.Offset = static_cast<DWORD>(24 + blobIndex * 32);
			if (!ReadFile(handle, entry, 32, &readBytes, &entryOverlapped) || readBytes != 32)
			{
				CloseHandle(handle);
				delete file;
				return CRIFS_IO_ERROR_NG_INVALID_DATA;
			}

			Uint32 blobID = 0;
			std::memcpy(&blobID, entry, 4);
			if (blobID != 1)
			{
				continue;
			}

			std::memcpy(&blobOffset, entry + 8, 8);
			std::memcpy(&blobEncryptedSize, entry + 16, 8);
			std::memcpy(&blobPlainSize, entry + 24, 8);
		}

		if (blobEncryptedSize <= 16)
		{
			CloseHandle(handle);
			delete file;
			return CRIFS_IO_ERROR_NG_NO_ENTRY;
		}

		/// [JP] 復号に使う最初の値を読み、中身の場所を覚えておく
		/// [EN] Read the starting value for decrypting and remember where the data is
		OVERLAPPED ivOverlapped{};
		ivOverlapped.Offset = static_cast<DWORD>(blobOffset & 0xFFFFFFFF);
		ivOverlapped.OffsetHigh = static_cast<DWORD>(blobOffset >> 32);
		if (!ReadFile(handle, file->iv_, 16, &readBytes, &ivOverlapped) || readBytes != 16)
		{
			CloseHandle(handle);
			delete file;
			return CRIFS_IO_ERROR_NG_INVALID_DATA;
		}

		file->ciphertextOffset_ = blobOffset + 16;
		file->ciphertextSize_ = blobEncryptedSize - 16;
		file->plainSize_ = blobPlainSize;

		*fileHandle = file;
		return CRIFS_IO_ERROR_OK;
	}

	/**
	* [JP]
	* 開いたファイルを閉じる。
	*
	* @brief  ファイルを閉じる
	* @param  fileHandle 閉じるファイル
	* @return いつも成功
	*
	* [EN]
	* Closes an opened file.
	*
	* @brief  Closes a file
	* @param  fileHandle The file to close
	* @return Always succeeds
	*/
	CriFsIoError CRIAPI AudioByteStream::Impl::Close(CriFsFileHn fileHandle)
	{
		File* file = static_cast<File*>(fileHandle);
		if (!file)
		{
			return CRIFS_IO_ERROR_OK;
		}

		if (file->handle_ != INVALID_HANDLE_VALUE)
		{
			CloseHandle(file->handle_);
		}

		delete file;
		return CRIFS_IO_ERROR_OK;
	}

	/**
	* [JP]
	* ファイルの大きさ（復号したあとの大きさ）を返す。
	*
	* @brief  ファイルの大きさを返す
	* @param  fileHandle 調べるファイル
	* @param  fileSize   大きさが入る
	* @return 調べられたかどうか
	*
	* [EN]
	* Returns the file size (the size after decrypting).
	*
	* @brief  Returns the file size
	* @param  fileHandle The file to check
	* @param  fileSize   Gets the size
	* @return Whether it worked
	*/
	CriFsIoError CRIAPI AudioByteStream::Impl::FileSize(CriFsFileHn fileHandle, CriSint64* fileSize)
	{
		File* file = static_cast<File*>(fileHandle);
		if (!file || !fileSize)
		{
			return CRIFS_IO_ERROR_NG;
		}

		*fileSize = static_cast<CriSint64>(file->plainSize_);
		return CRIFS_IO_ERROR_OK;
	}

	/**
	* [JP]
	* ファイルの一部を読む。暗号化されていれば、その部分だけ復号する。
	*
	* @brief  ファイルの一部を読む
	* @param  fileHandle 読むファイル
	* @param  offset     読み始める場所
	* @param  readSize   読みたい大きさ
	* @param  buffer     読んだデータが入る
	* @param  bufferSize buffer の大きさ
	* @return 読めたかどうか
	*
	* [EN]
	* Reads part of a file, decrypting only that part if it is encrypted.
	*
	* @brief  Reads part of a file
	* @param  fileHandle The file to read
	* @param  offset     Where to start reading
	* @param  readSize   How much to read
	* @param  buffer     Gets the data read
	* @param  bufferSize The size of buffer
	* @return Whether it read
	*/
	CriFsIoError CRIAPI AudioByteStream::Impl::Read(CriFsFileHn fileHandle, CriSint64 offset, CriSint64 readSize, void* buffer, CriSint64 bufferSize)
	{
		File* file = static_cast<File*>(fileHandle);
		if (!file || !buffer)
		{
			return CRIFS_IO_ERROR_NG;
		}

		file->readSize_ = 0;

		if (offset < 0 || readSize <= 0 || static_cast<Uint64>(offset) >= file->plainSize_)
		{
			return CRIFS_IO_ERROR_OK;
		}

		/// [JP] ファイルの終わりや buffer を超えないように、読む大きさを決める
		/// [EN] Decide how much to read without going past the file end or the buffer
		Uint64 toRead = Min<Uint64>(static_cast<Uint64>(readSize), file->plainSize_ - static_cast<Uint64>(offset));
		toRead = Min<Uint64>(toRead, static_cast<Uint64>(bufferSize));

		DWORD readBytes = 0;

		/// [JP] 暗号化されていなければそのまま読む
		/// [EN] If it is not encrypted, just read it
		if (!file->encrypted_)
		{
			OVERLAPPED overlapped{};
			overlapped.Offset = static_cast<DWORD>(offset & 0xFFFFFFFF);
			overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32);
			if (!ReadFile(file->handle_, buffer, static_cast<DWORD>(toRead), &readBytes, &overlapped))
			{
				return CRIFS_IO_ERROR_NG;
			}

			file->readSize_ = readBytes;
			return CRIFS_IO_ERROR_OK;
		}

		/// [JP] 16 バイトの区切りに合わせて、読む範囲を広げる
		/// [EN] Widen the range to line up with 16-byte blocks
		Uint64 blockStart = (static_cast<Uint64>(offset) / 16) * 16;
		Uint64 blockEnd = Min<Uint64>(((static_cast<Uint64>(offset) + toRead + 15) / 16) * 16, file->ciphertextSize_);

		DynamicArray<Byte> chainIv(16);
		if (blockStart == 0)
		{
			std::memcpy(chainIv.data(), file->iv_, 16);
		}
		else
		{
			Uint64 chainOffset = file->ciphertextOffset_ + blockStart - 16;
			OVERLAPPED chainOverlapped{};
			chainOverlapped.Offset = static_cast<DWORD>(chainOffset & 0xFFFFFFFF);
			chainOverlapped.OffsetHigh = static_cast<DWORD>(chainOffset >> 32);
			if (!ReadFile(file->handle_, chainIv.data(), 16, &readBytes, &chainOverlapped) || readBytes != 16)
			{
				return CRIFS_IO_ERROR_NG;
			}
		}

		/// [JP] 暗号化された部分を読んで復号する
		/// [EN] Read the encrypted part and decrypt it
		DynamicArray<Byte> ciphertextChunk(static_cast<Size>(blockEnd - blockStart));
		Uint64 chunkOffset = file->ciphertextOffset_ + blockStart;
		OVERLAPPED chunkOverlapped{};
		chunkOverlapped.Offset = static_cast<DWORD>(chunkOffset & 0xFFFFFFFF);
		chunkOverlapped.OffsetHigh = static_cast<DWORD>(chunkOffset >> 32);
		if (!ReadFile(file->handle_, ciphertextChunk.data(), static_cast<DWORD>(ciphertextChunk.size()), &readBytes, &chunkOverlapped) || readBytes != ciphertextChunk.size())
		{
			return CRIFS_IO_ERROR_NG;
		}

		static const DynamicArray<Byte> key = Sha256::Hash(reinterpret_cast<const Byte*>(SC_ENCRYPTION_KEY_SEED), std::strlen(SC_ENCRYPTION_KEY_SEED));

		DynamicArray<Byte> decrypted = Aes256::DecryptUnpadded(key, chainIv, ciphertextChunk);
		if (decrypted.empty())
		{
			return CRIFS_IO_ERROR_NG;
		}

		/// [JP] 復号した中から、頼まれた部分だけを写す
		/// [EN] Copy out just the part that was asked for
		Uint64 offsetInChunk = static_cast<Uint64>(offset) - blockStart;
		std::memcpy(buffer, decrypted.data() + offsetInChunk, static_cast<Size>(toRead));

		file->readSize_ = toRead;
		return CRIFS_IO_ERROR_OK;
	}

	/**
	* [JP]
	* 読み込みが終わったかを答える。読み込みはすぐ終わるので、いつも終わっている。
	*
	* @brief  読み込みが終わったか答える
	* @param  fileHandle 調べるファイル
	* @param  result     終わっていれば CRI_TRUE が入る
	* @return 調べられたかどうか
	*
	* [EN]
	* Answers whether reading has finished; reading is instant, so it always has.
	*
	* @brief  Answers if reading is done
	* @param  fileHandle The file to check
	* @param  result     Gets CRI_TRUE when done
	* @return Whether the check worked
	*/
	CriFsIoError CRIAPI AudioByteStream::Impl::Complete(CriFsFileHn fileHandle, CriBool* result)
	{
		if (!result)
		{
			return CRIFS_IO_ERROR_NG;
		}

		*result = CRI_TRUE;
		return CRIFS_IO_ERROR_OK;
	}

	/**
	* [JP]
	* 最後に読めた大きさを返す。
	*
	* @brief  読めた大きさを返す
	* @param  fileHandle 調べるファイル
	* @param  readSize   読めた大きさが入る
	* @return 調べられたかどうか
	*
	* [EN]
	* Returns how much was read last time.
	*
	* @brief  Returns the amount read
	* @param  fileHandle The file to check
	* @param  readSize   Gets the amount read
	* @return Whether it worked
	*/
	CriFsIoError CRIAPI AudioByteStream::Impl::ReadSize(CriFsFileHn fileHandle, CriSint64* readSize)
	{
		File* file = static_cast<File*>(fileHandle);
		if (!file || !readSize)
		{
			return CRIFS_IO_ERROR_NG;
		}

		*readSize = static_cast<CriSint64>(file->readSize_);
		return CRIFS_IO_ERROR_OK;
	}

	/**
	* [JP]
	* メモリから読む仕組みを音のライブラリに登録する。
	*
	* @brief 仕組みを登録する
	*
	* [EN]
	* Registers the memory reader with the sound library.
	*
	* @brief Registers the reader
	*/
	void AudioByteStream::Install()
	{
		criFs_SetSelectIoCallback(&Impl::SelectIo);
	}

	/**
	* [JP]
	* 登録した仕組みを外す。
	*
	* @brief 仕組みを外す
	*
	* [EN]
	* Removes the registered reader.
	*
	* @brief Removes the reader
	*/
	void AudioByteStream::Uninstall()
	{
		criFs_SetSelectIoCallback(nullptr);
	}
}
