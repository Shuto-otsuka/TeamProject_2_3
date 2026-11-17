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
	* [EN]
	* Initializes COM and Media Foundation for MP3 decoding.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* MP3 のデコードに使う COM と Media Foundation を初期化する。
	*/
	AudioLoader::AudioLoader()
	{
		/// [EN] Track successful initialization so this instance balances only the calls it owns.
		/// [JP] このインスタンスが所有する初期化呼び出しだけを終了できるよう、成功状態を保持する。
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
	* [EN]
	* Shuts down the facilities initialized by this loader.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このローダーが初期化した機能を終了する。
	*/
	AudioLoader::~AudioLoader()
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
	* Loads a Sound from filePath. A ".audio" path is read as the
	* encrypted cache; a source (".wav"/".mp3"/".acb") is converted in
	* memory, with an ".acb" streaming from the ".awb" beside it when
	* there is one. Nothing is written. Returns a null handle on failure.
	* thread is unused: a Sound needs no per-thread state.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath から Sound を読み込む。".audio" は暗号化されたキャッシュ
	* として読む。素材（".wav"/".mp3"/".acb"）はメモリ上で変換し、".acb"
	* は隣に ".awb" があればそこからストリーミングする。何も書き出さない。
	* 失敗時は null ハンドルを返す。thread は使わない（Sound はスレッド
	* ごとの状態を持たないため）。
	*/
	Handle<Sound> AudioLoader::Load(String filePath, LoadThread thread)
	{
		/// [EN] A ".awb" is only the streamed half of an ".acb", so it is never loaded on its own.
		/// [JP] ".awb" は ".acb" のストリーミング側でしかないため、単体では読み込まない。
		std::filesystem::path fileFsPath(filePath.c_str());
		std::string extension = fileFsPath.extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });

		if (extension == ".awb")
		{
			return Handle<Sound>::null();
		}

		/// [EN] Decode the file into its plain body, its kind, and the path CRI streams the AWB from.
		/// [JP] ファイルを、平文の本体・その種別・CRI が AWB をストリーミングするパスへデコードする。
		SoundType type = SoundType::Wave;
		DynamicArray<Byte> data;
		String awbPath;
		if (!AudioDecoder::Decode(filePath, type, data, awbPath))
		{
			SC_LOG_ERROR("AudioLoader: 音声を読み込めませんでした ({})", fileFsPath.string());
			return Handle<Sound>::null();
		}

		/// [EN] Store the plain body in a stable Sound slot addressed by the returned handle.
		/// [JP] 平文の本体を、返却ハンドルで参照する安定した Sound スロットへ格納する。
		Handle<Sound> handle = pool_.Create();
		Sound* sound = pool_.Get(handle);
		if (!sound)
		{
			return Handle<Sound>::null();
		}

		sound->type_ = type;
		sound->data_ = std::move(data);
		sound->awbPath_ = awbPath;

		/// [EN] Register cue-sheet data with CRI when the global ACF is available.
		/// [JP] グローバル ACF を利用できる場合は、キューシートデータを CRI へ登録する。
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
	* [EN]
	* Releases CRI data associated with a Sound and destroys its handle.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Sound に関連する CRI データを解放し、ハンドルを破棄する。
	*/
	void AudioLoader::Clear(Handle<Sound>& handle)noexcept
	{
		/// [EN] Release the ACB before returning its backing Sound slot to the pool.
		/// [JP] Sound の格納スロットをプールへ返す前に、ACB を解放する。
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
	* [EN]
	* Converts a supported audio source into an encrypted .audio cache.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 対応する音声素材を暗号化された .audio キャッシュへ変換する。
	*/
	Bool AudioLoader::Bake(String sourcePath, String cachePath)
	{
		/// [EN] Decode the source into the plain main body, the same bytes Load gives a Sound.
		/// [JP] 素材を平文の本体データへデコードする。Load が Sound に渡すのと同じバイト列。
		SoundType type = SoundType::Wave;
		DynamicArray<Byte> mainData;
		String awbPath;
		if (!AudioDecoder::Decode(sourcePath, type, mainData, awbPath))
		{
			return false;
		}

		/// [EN] Package the AWB beside an ".acb" as the optional streamed-data blob when present.
		/// [JP] ".acb" の隣に AWB がある場合は、任意のストリームデータブロブとして格納する。
		DynamicArray<Byte> streamData;
		if (!awbPath.str().empty())
		{
			DynamicArray<Uint8> streamBytes = FileUtility::LoadFileBinary(awbPath);
			streamData.resize(streamBytes.size());
			std::memcpy(streamData.data(), streamBytes.data(), streamBytes.size());
		}

		/// [EN] Encrypt each populated blob with an independent initialization vector.
		/// [JP] 格納する各ブロブを、それぞれ独立した初期化ベクトルで暗号化する。
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

		/// [EN] Calculate the container header and aligned blob layout.
		/// [JP] コンテナのヘッダーと、境界調整したブロブ配置を算出する。
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

		/// [EN] Write metadata followed by the encrypted main and optional stream blobs.
		/// [JP] メタデータに続けて、暗号化した本体と任意のストリームブロブを書き込む。
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
