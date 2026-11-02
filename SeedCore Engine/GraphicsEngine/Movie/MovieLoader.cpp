#include <GraphicsEngine/Movie/MovieLoader.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Log/Error.h>
#include <FoundationEngine/File/FileUtility.h>
#include <FoundationEngine/Serialization/Encryption/Aes256.h>
#include <FoundationEngine/Serialization/Encryption/Sha256.h>

namespace SeedCore
{
	MovieLoader::MovieLoader()
	{
		HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		ownsComInitialize_ = (comResult == S_OK || comResult == S_FALSE);

		HRESULT mfResult = MFStartup(MF_VERSION, MFSTARTUP_LITE);
		mfStarted_ = SUCCEEDED(mfResult);

		if (!mfStarted_)
		{
			SC_LOG_WARNING("MovieLoader: MFStartupに失敗しました - Movie機能は無効です");
		}
	}

	MovieLoader::~MovieLoader()
	{
		if (mfStarted_)
		{
			MFShutdown();
		}

		if (ownsComInitialize_)
		{
			CoUninitialize();
		}
	}

	/**
	* [EN]
	* Opens the video at filePath (a source, or a ".movie" cache) and
	* returns the handle of the new Video, or a null handle when Media
	* Foundation is unavailable or the video cannot be opened. A source
	* is read from its sibling ".movie" cache when that is newer. thread
	* is unused: the decoder keeps no per-thread state.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath の動画（ソースか ".movie" キャッシュ）を開き、新しい Video の
	* ハンドルを返す。Media Foundation が使えない、または動画を開けない
	* 場合は null のハンドルを返す。ソースは、隣の ".movie" キャッシュの
	* 方が新しければそちらから読む。thread は使わない（デコーダはスレッド
	* ごとの状態を持たないため）。
	*/
	Handle<Video> MovieLoader::Load(String filePath, LoadThread thread)
	{
		if (!mfStarted_)
		{
			return Handle<Video>::null();
		}

		Handle<Video> handle = pool_.Create();
		Video* video = pool_.Get(handle);
		if (!video)
		{
			return Handle<Video>::null();
		}

		if (!video->decoder_.Initialize(filePath.str()))
		{
			SC_LOG_ERROR("MovieLoader: 動画の読み込みに失敗しました: {}", filePath.str());
			Clear(handle);
			return Handle<Video>::null();
		}

		video->textureWidth_ = video->decoder_.GetWidth();
		video->textureHeight_ = video->decoder_.GetHeight();

		video->playbackTime_ = 0.0;
		video->hasPendingFrame_ = false;
		video->hasLastUpdateTime_ = false;
		video->autoPlayStarted_ = false;

		return handle;
	}

	/**
	* [EN]
	* Tears the decoder and the GPU resources of the Video handle names
	* down, then frees its slot.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* handle が示す Video のデコーダと GPU リソースを破棄してから、
	* スロットを解放する。
	*/
	void MovieLoader::Clear(Handle<Video>& handle)noexcept
	{
		Video* video = pool_.Get(handle);
		if (video)
		{
			video->decoder_.Finalize();

			video->frameTexture_.Reset();
			video->uploadBuffers_[0].Reset();
			video->uploadBuffers_[1].Reset();
			video->alignedRowPitch_ = 0;
			video->uploadBufferParity_ = 0;
			video->frameTextureIndexAllocated_ = false;
			video->frameDirty_ = false;
			video->textureWidth_ = 0;
			video->textureHeight_ = 0;
			video->playbackTime_ = 0.0;
			video->pendingFrameTime_ = 0.0;
			video->hasPendingFrame_ = false;
			video->playing_ = false;
			video->autoPlayStarted_ = false;
			video->hasLastUpdateTime_ = false;
		}

		pool_.Destroy(handle);
	}

	/**
	* [EN]
	* Writes the bytes of the source video at sourcePath, AES-256-CBC
	* encrypted behind a random 16-byte IV, to the ".movie" cache at
	* cachePath. The cache has no field framing, so the decoder can
	* decrypt any requested range without parsing the whole file.
	* Returns false when the source cannot be read or the write failed.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* sourcePath のソース動画のバイト列を、ランダムな 16 バイトの IV の後に
	* AES-256-CBC で暗号化して、cachePath の ".movie" キャッシュへ書く。
	* キャッシュはフィールド形式を持たないため、デコーダはファイル全体を
	* 解析せずに任意の範囲を復号できる。ソースを読めない、または書き込みに
	* 失敗した場合は false を返す。
	*/
	Bool MovieLoader::Bake(String sourcePath, String cachePath)
	{
		DynamicArray<Uint8> sourceBytes = FileUtility::LoadFileBinary(sourcePath);
		if (sourceBytes.empty())
		{
			return false;
		}

		DynamicArray<Byte> plaintext(sourceBytes.size());
		std::memcpy(plaintext.data(), sourceBytes.data(), sourceBytes.size());

		/// [EN] The key is derived from the engine's key seed, the same one every other encrypted cache uses.
		/// [JP] 鍵はエンジンの鍵シードから作る。他の暗号化キャッシュと同じもの。
		DynamicArray<Byte> key = Sha256::Hash(reinterpret_cast<const Byte*>(SC_ENCRYPTION_KEY_SEED), std::strlen(SC_ENCRYPTION_KEY_SEED));

		/// [EN] A fresh random IV per file, written in front of the ciphertext so the decoder can read it back.
		/// [JP] ファイルごとに新しいランダムな IV。デコーダが読み戻せるよう、暗号文の前に書く。
		DynamicArray<Byte> iv(16);
		std::random_device randomDevice;
		for (Byte& ivByte : iv)
		{
			ivByte = static_cast<Byte>(randomDevice());
		}

		DynamicArray<Byte> ciphertext = Aes256::Encrypt(key, iv, plaintext);

		std::ofstream ofs(std::filesystem::path(cachePath.w_str()), std::ios::binary);
		if (!ofs)
		{
			return false;
		}
		ofs.write(iv.data(), iv.size());
		ofs.write(ciphertext.data(), ciphertext.size());
		return static_cast<Bool>(ofs);
	}
}
