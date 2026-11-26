#include <External/CRI/Include/cri_adx2le.h>
#include <AudioEngine/Audio/AudioLoader.h>
#include <AudioEngine/Audio/AudioDecoder.h>
#include <FoundationEngine/File/FileUtility.h>
#include <FoundationEngine/Serialization/Encryption/Aes256.h>
#include <FoundationEngine/Serialization/Encryption/Sha256.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	/**
	* [JP]
	* 音のファイルを読む準備をする。
	*
	* @brief 読む準備をする
	*
	* [EN]
	* Gets ready to read sound files.
	*
	* @brief Gets ready to read
	*/
	AudioLoader::AudioLoader()
	{
		/// [JP] mp3 を変換するために COM と Media Foundation を始める
		/// [EN] Start COM and Media Foundation so mp3 files can be converted
		HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		ownsComInitialize_ = (comResult == S_OK || comResult == S_FALSE);

		HRESULT mfResult = MFStartup(MF_VERSION, MFSTARTUP_LITE);
		mfStarted_ = SUCCEEDED(mfResult);

		if (!mfStarted_)
		{
			SC_LOG_WARNING("AudioLoader: MFStartupに失敗しました - mp3の変換は無効です");
		}
	}

	/**
	* [JP]
	* 読み込みに使ったものを片付ける。
	*
	* @brief 片付ける
	*
	* [EN]
	* Cleans up what was used for reading.
	*
	* @brief Cleans up
	*/
	AudioLoader::~AudioLoader()
	{
		/// [JP] 始めたものだけを終わらせる
		/// [EN] Shut down only what was started
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
	* [JP]
	* ファイルを読み込んで音を作る。
	*
	* @brief  ファイルを読み込む
	* @param  filePath 読むファイル
	* @param  thread   読み込むスレッド
	* @return 作った音のハンドル。失敗したら空
	*
	* [EN]
	* Reads a file and makes a sound.
	*
	* @brief  Reads a file
	* @param  filePath The file to read
	* @param  thread   The thread to read on
	* @return A handle to the new sound, or empty on failure
	*/
	Handle<Sound> AudioLoader::Load(String filePath, LoadThread thread)
	{
		/// [JP] AWB は単独では読まない（ACB と一緒に使う）
		/// [EN] An AWB is never read on its own (it is used with its ACB)
		std::filesystem::path fileFsPath(filePath.c_str());
		std::string extension = fileFsPath.extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });

		if (extension == ".awb")
		{
			return Handle<Sound>::null();
		}

		/// [JP] ファイルを鳴らせるデータに変える
		/// [EN] Turn the file into playable data
		SoundType type = SoundType::Wave;
		DynamicArray<Byte> data;
		String awbPath;
		if (!AudioDecoder::Decode(filePath, type, data, awbPath))
		{
			SC_LOG_ERROR("AudioLoader: 音声を読み込めませんでした ({})", fileFsPath.string());
			return Handle<Sound>::null();
		}

		/// [JP] 音を作ってデータを入れる
		/// [EN] Make a sound and fill in the data
		Handle<Sound> handle = pool_.Create();
		Sound* sound = pool_.Get(handle);
		if (!sound)
		{
			return Handle<Sound>::null();
		}

		sound->type_ = type;
		sound->data_ = std::move(data);
		sound->awbPath_ = awbPath;

		/// [JP] キューシートなら音のライブラリに登録する
		/// [EN] Register a cue sheet with the sound library
		if (sound->type_ == SoundType::CueSheet)
		{
			CriAtomExAcfInfo acfInfo{};
			if (criAtomExAcf_GetAcfInfo(&acfInfo) != CRI_TRUE)
			{
				SC_LOG_WARNING("AudioLoader: ACFが登録されていないため、キューシートをCRIへ登録できません ({})", fileFsPath.string());
			}
			else
			{
				sound->acbHandle_ = criAtomExAcb_LoadAcbData(sound->data_.data(), static_cast<CriSint32>(sound->data_.size()), nullptr, sound->awbPath_.str().empty() ? nullptr : sound->awbPath_.c_str(), nullptr, 0);
				if (!sound->acbHandle_)
				{
					SC_LOG_ERROR("AudioLoader: ACBの読み込みに失敗しました ({})", fileFsPath.string());
					pool_.Destroy(handle);
					return Handle<Sound>::null();
				}
			}
		}

		return handle;
	}

	/**
	* [JP]
	* 読み込んだ音を消す。
	*
	* @brief 音を消す
	* @param handle 消す音のハンドル
	*
	* [EN]
	* Throws away a loaded sound.
	*
	* @brief Removes a sound
	* @param handle The handle of the sound to remove
	*/
	void AudioLoader::Clear(Handle<Sound>& handle)noexcept
	{
		/// [JP] キューシートの登録を外してから音を消す
		/// [EN] Unregister the cue sheet, then remove the sound
		if (Sound* sound = pool_.Get(handle))
		{
			if (sound->acbHandle_)
			{
				criAtomExAcb_Release(sound->acbHandle_);
				sound->acbHandle_ = nullptr;
			}
		}

		pool_.Destroy(handle);
	}

	/**
	* [JP]
	* 元のファイルを、すぐ読める形（キャッシュ）にして保存する。
	*
	* @brief  キャッシュを作る
	* @param  sourcePath 元のファイル
	* @param  cachePath  保存する場所
	* @return うまくいったら true
	*
	* [EN]
	* Saves the original file in a quick-to-read form (a cache).
	*
	* @brief  Makes a cache
	* @param  sourcePath The original file
	* @param  cachePath  Where to save it
	* @return True if it worked
	*/
	Bool AudioLoader::Bake(String sourcePath, String cachePath)
	{
		/// [JP] 元のファイルと、あれば AWB を読む
		/// [EN] Read the original file and its AWB if there is one
		SoundType type = SoundType::Wave;
		DynamicArray<Byte> mainData;
		String awbPath;
		if (!AudioDecoder::Decode(sourcePath, type, mainData, awbPath))
		{
			return false;
		}

		DynamicArray<Byte> streamData;
		if (!awbPath.str().empty())
		{
			DynamicArray<Uint8> streamBytes = FileUtility::LoadFileBinary(awbPath);
			streamData.resize(streamBytes.size());
			std::memcpy(streamData.data(), streamBytes.data(), streamBytes.size());
		}

		/// [JP] 鍵と毎回違う初期値で、中身を暗号化する
		/// [EN] Encrypt the contents with the key and a fresh starting value
		static const DynamicArray<Byte> key = Sha256::Hash(reinterpret_cast<const Byte*>(SC_ENCRYPTION_KEY_SEED), std::strlen(SC_ENCRYPTION_KEY_SEED));
		std::random_device randomDevice;

		DynamicArray<Byte> mainIv(16);
		for (Byte& ivByte : mainIv)
		{
			ivByte = static_cast<Byte>(randomDevice());
		}
		DynamicArray<Byte> mainCiphertext = Aes256::Encrypt(key, mainIv, mainData);

		DynamicArray<Byte> streamIv(16);
		DynamicArray<Byte> streamCiphertext;
		if (!streamData.empty())
		{
			for (Byte& ivByte : streamIv)
			{
				ivByte = static_cast<Byte>(randomDevice());
			}
			streamCiphertext = Aes256::Encrypt(key, streamIv, streamData);
		}

		/// [JP] ファイルの中のどこに何を置くかを決める
		/// [EN] Decide where each part goes in the file
		Uint32 version = 1;
		Uint32 typeValue = static_cast<Uint32>(type);
		Uint32 blobCount = streamData.empty() ? 1 : 2;
		Uint32 reserved = 0;

		Uint64 mainOffset = 24 + static_cast<Uint64>(blobCount) * 32;
		Uint64 mainEncryptedSize = 16 + mainCiphertext.size();
		Uint64 mainPlainSize = mainData.size();

		Uint64 streamOffset = ((mainOffset + mainEncryptedSize + 4095) / 4096) * 4096;
		Uint64 streamEncryptedSize = streamCiphertext.empty() ? 0 : 16 + streamCiphertext.size();
		Uint64 streamPlainSize = streamData.size();

		/// [JP] 見出し（種類や数）を書く
		/// [EN] Write the header (kind and counts)
		std::ofstream ofs(cachePath.c_str(), std::ios::binary);
		if (!ofs)
		{
			return false;
		}

		ofs.write("SCAUDIO\0", 8);
		ofs.write(reinterpret_cast<const Char*>(&version), 4);
		ofs.write(reinterpret_cast<const Char*>(&typeValue), 4);
		ofs.write(reinterpret_cast<const Char*>(&blobCount), 4);
		ofs.write(reinterpret_cast<const Char*>(&reserved), 4);

		/// [JP] それぞれの中身の場所と大きさを書く
		/// [EN] Write where each part is and how big it is
		Uint32 mainBlobID = 0;
		ofs.write(reinterpret_cast<const Char*>(&mainBlobID), 4);
		ofs.write(reinterpret_cast<const Char*>(&reserved), 4);
		ofs.write(reinterpret_cast<const Char*>(&mainOffset), 8);
		ofs.write(reinterpret_cast<const Char*>(&mainEncryptedSize), 8);
		ofs.write(reinterpret_cast<const Char*>(&mainPlainSize), 8);

		if (blobCount == 2)
		{
			Uint32 streamBlobID = 1;
			ofs.write(reinterpret_cast<const Char*>(&streamBlobID), 4);
			ofs.write(reinterpret_cast<const Char*>(&reserved), 4);
			ofs.write(reinterpret_cast<const Char*>(&streamOffset), 8);
			ofs.write(reinterpret_cast<const Char*>(&streamEncryptedSize), 8);
			ofs.write(reinterpret_cast<const Char*>(&streamPlainSize), 8);
		}

		/// [JP] 暗号化した中身を書く。AWB の分は区切りのいい場所から書く
		/// [EN] Write the encrypted parts; the AWB part starts at a tidy boundary
		ofs.write(mainIv.data(), static_cast<std::streamsize>(mainIv.size()));
		ofs.write(mainCiphertext.data(), static_cast<std::streamsize>(mainCiphertext.size()));

		if (blobCount == 2)
		{
			DynamicArray<Byte> padding(static_cast<Size>(streamOffset - (mainOffset + mainEncryptedSize)), Byte(0));
			ofs.write(padding.data(), static_cast<std::streamsize>(padding.size()));
			ofs.write(streamIv.data(), static_cast<std::streamsize>(streamIv.size()));
			ofs.write(streamCiphertext.data(), static_cast<std::streamsize>(streamCiphertext.size()));
		}

		return static_cast<Bool>(ofs);
	}
}
