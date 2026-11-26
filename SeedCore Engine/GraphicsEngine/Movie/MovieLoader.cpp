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

	Bool MovieLoader::Bake(String sourcePath, String cachePath)
	{
		DynamicArray<Uint8> sourceBytes = FileUtility::LoadFileBinary(sourcePath);
		if (sourceBytes.empty())
		{
			return false;
		}

		DynamicArray<Byte> plaintext(sourceBytes.size());
		std::memcpy(plaintext.data(), sourceBytes.data(), sourceBytes.size());

		DynamicArray<Byte> key = Sha256::Hash(reinterpret_cast<const Byte*>(SC_ENCRYPTION_KEY_SEED), std::strlen(SC_ENCRYPTION_KEY_SEED));

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
