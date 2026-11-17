#include <AudioEngine/Audio/AudioDecoder.h>
#include <FoundationEngine/File/FileUtility.h>
#include <FoundationEngine/Serialization/Encryption/Aes256.h>
#include <FoundationEngine/Serialization/Encryption/Sha256.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	/**
	* [EN]
	* Decodes the audio file at filePath. type receives whether the
	* bytes are a cue sheet or a wave, data the plain body, and awbPath
	* the path CRI streams the AWB from (empty when there is none): the
	* cache itself for a ".audio" holding a stream blob, or the plain
	* ".awb" beside an ".acb". An ".mp3" is decoded to PCM and wrapped
	* in a wave container. Returns false when the file cannot be read
	* or its extension is not supported.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* filePath の音声ファイルをデコードする。type にはキューシートか
	* 波形か、data には平文の本体、awbPath には CRI が AWB を
	* ストリーミングするパス（無ければ空）が入る。awbPath は、ストリーム
	* ブロブを持つ ".audio" ならキャッシュ自身、".acb" なら隣の素の
	* ".awb"。".mp3" は PCM へデコードして wave コンテナに包む。ファイルを
	* 読めない、または拡張子が対応していない場合は false を返す。
	*/
	Bool AudioDecoder::Decode(String filePath, SoundType& type, DynamicArray<Byte>& data, String& awbPath)
	{
		/// [EN] Select the decoding path from the file extension.
		/// [JP] ファイルの拡張子に応じてデコード経路を選択する。
		std::filesystem::path fileFsPath(filePath.c_str());
		std::string extension = fileFsPath.extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });

		awbPath = String();

		if (extension == ".audio")
		{
			/// [EN] Read and validate the fixed header of the encrypted audio container.
			/// [JP] 暗号化音声コンテナの固定ヘッダーを読み込み、妥当性を検証する。
			std::ifstream ifs(fileFsPath, std::ios::binary);
			if (!ifs)
			{
				SC_LOG_ERROR("AudioDecoder: .audioを開けませんでした ({})", fileFsPath.string());
				return false;
			}

			Byte magic[8]{};
			Uint32 version = 0;
			Uint32 typeValue = 0;
			Uint32 blobCount = 0;
			Uint32 reserved = 0;
			ifs.read(magic, 8);
			ifs.read(reinterpret_cast<Char*>(&version), 4);
			ifs.read(reinterpret_cast<Char*>(&typeValue), 4);
			ifs.read(reinterpret_cast<Char*>(&blobCount), 4);
			ifs.read(reinterpret_cast<Char*>(&reserved), 4);

			if (!ifs || std::memcmp(magic, "SCAUDIO\0", 8) != 0 || version != 1 || blobCount == 0 || blobCount > 2 || typeValue > static_cast<Uint32>(SoundType::Wave))
			{
				SC_LOG_ERROR("AudioDecoder: .audioのヘッダーが不正です ({})", fileFsPath.string());
				return false;
			}

			/// [EN] Locate the in-memory body and record whether a streamed AWB blob is present.
			/// [JP] メモリへ読む本体の位置を取得し、ストリーミング用 AWB ブロブの有無を記録する。
			Uint64 mainOffset = 0;
			Uint64 mainEncryptedSize = 0;
			Bool hasStreamBlob = false;
			for (Uint32 blobIndex = 0; blobIndex < blobCount; ++blobIndex)
			{
				Uint32 blobID = 0;
				Uint32 blobReserved = 0;
				Uint64 offset = 0;
				Uint64 encryptedSize = 0;
				Uint64 plainSize = 0;
				ifs.read(reinterpret_cast<Char*>(&blobID), 4);
				ifs.read(reinterpret_cast<Char*>(&blobReserved), 4);
				ifs.read(reinterpret_cast<Char*>(&offset), 8);
				ifs.read(reinterpret_cast<Char*>(&encryptedSize), 8);
				ifs.read(reinterpret_cast<Char*>(&plainSize), 8);

				if (blobID == 0)
				{
					mainOffset = offset;
					mainEncryptedSize = encryptedSize;
				}
				else if (blobID == 1)
				{
					hasStreamBlob = true;
				}
			}

			if (!ifs || mainEncryptedSize <= 16)
			{
				SC_LOG_ERROR("AudioDecoder: .audioの本体データが不正です ({})", fileFsPath.string());
				return false;
			}

			/// [EN] Read the initialization vector and ciphertext, then decrypt the main body.
			/// [JP] 初期化ベクトルと暗号文を読み込み、本体データを復号する。
			DynamicArray<Byte> iv(16);
			DynamicArray<Byte> ciphertext(static_cast<Size>(mainEncryptedSize - 16));
			ifs.seekg(static_cast<std::streamoff>(mainOffset));
			ifs.read(iv.data(), 16);
			ifs.read(ciphertext.data(), static_cast<std::streamsize>(ciphertext.size()));
			if (!ifs)
			{
				SC_LOG_ERROR("AudioDecoder: .audioの本体データを読み込めませんでした ({})", fileFsPath.string());
				return false;
			}

			static const DynamicArray<Byte> key = Sha256::Hash(reinterpret_cast<const Byte*>(SC_ENCRYPTION_KEY_SEED), std::strlen(SC_ENCRYPTION_KEY_SEED));
			data = Aes256::Decrypt(key, iv, ciphertext);
			if (data.empty())
			{
				SC_LOG_ERROR("AudioDecoder: .audioの復号に失敗しました ({})", fileFsPath.string());
				return false;
			}

			type = static_cast<SoundType>(typeValue);

			/// [EN] The streamed AWB stays inside the cache; the custom file I/O decrypts only the ranges CRI asks for.
			/// [JP] ストリーミングする AWB はキャッシュの中に残し、独自のファイル I/O が CRI の求めた範囲だけを復号する。
			if (hasStreamBlob)
			{
				awbPath = String("seedcore_audio://" + std::filesystem::absolute(fileFsPath).generic_string());
			}
			return true;
		}
		else if (extension == ".acb" || extension == ".wav")
		{
			/// [EN] Preserve ACB and wave bytes directly as the main body.
			/// [JP] ACB と wave のバイト列は、そのまま本体データとして保持する。
			type = (extension == ".acb") ? SoundType::CueSheet : SoundType::Wave;

			DynamicArray<Uint8> sourceBytes = FileUtility::LoadFileBinary(String(fileFsPath.string()));
			if (sourceBytes.empty())
			{
				SC_LOG_ERROR("AudioDecoder: 音声ファイルを読み込めませんでした ({})", fileFsPath.string());
				return false;
			}
			data.resize(sourceBytes.size());
			std::memcpy(data.data(), sourceBytes.data(), sourceBytes.size());

			/// [EN] An ".acb" streams from the plain ".awb" beside it.
			/// [JP] ".acb" は隣の素の ".awb" からストリーミングする。
			if (extension == ".acb")
			{
				std::filesystem::path awbFsPath = fileFsPath;
				awbFsPath.replace_extension(".awb");
				if (std::filesystem::exists(awbFsPath))
				{
					awbPath = String(std::filesystem::absolute(awbFsPath).generic_string());
				}
			}
			return true;
		}
		else if (extension == ".mp3")
		{
			/// [EN] Decode MP3 input to PCM through Media Foundation.
			/// [JP] Media Foundation を通じて MP3 入力を PCM へデコードする。
			std::wstring widePath = fileFsPath.wstring();
			Microsoft::WRL::ComPtr<IMFSourceReader> sourceReader;
			if (FAILED(MFCreateSourceReaderFromURL(widePath.c_str(), nullptr, sourceReader.ReleaseAndGetAddressOf())))
			{
				SC_LOG_ERROR("AudioDecoder: mp3を開けませんでした ({})", fileFsPath.string());
				return false;
			}

			sourceReader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
			sourceReader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);

			Microsoft::WRL::ComPtr<IMFMediaType> outputType;
			if (FAILED(MFCreateMediaType(outputType.ReleaseAndGetAddressOf())))
			{
				return false;
			}
			outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
			outputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);

			if (FAILED(sourceReader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr, outputType.Get())))
			{
				SC_LOG_ERROR("AudioDecoder: mp3のPCM出力を設定できませんでした ({})", fileFsPath.string());
				return false;
			}

			Microsoft::WRL::ComPtr<IMFMediaType> currentType;
			if (FAILED(sourceReader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), currentType.ReleaseAndGetAddressOf())))
			{
				return false;
			}

			WAVEFORMATEX* waveFormat = nullptr;
			Uint32 waveFormatSize = 0;
			if (FAILED(MFCreateWaveFormatExFromMFMediaType(currentType.Get(), &waveFormat, &waveFormatSize)))
			{
				return false;
			}

			Uint16 channels = waveFormat->nChannels;
			Uint32 samplesPerSecond = waveFormat->nSamplesPerSec;
			Uint16 bitsPerSample = waveFormat->wBitsPerSample;
			CoTaskMemFree(waveFormat);

			DynamicArray<Byte> pcmData;
			while (true)
			{
				DWORD streamFlags = 0;
				Microsoft::WRL::ComPtr<IMFSample> sample;
				if (FAILED(sourceReader->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, nullptr, &streamFlags, nullptr, sample.ReleaseAndGetAddressOf())))
				{
					break;
				}

				if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
				{
					break;
				}

				if (!sample)
				{
					continue;
				}

				Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
				if (FAILED(sample->ConvertToContiguousBuffer(buffer.ReleaseAndGetAddressOf())))
				{
					break;
				}

				BYTE* sampleData = nullptr;
				DWORD currentLength = 0;
				if (FAILED(buffer->Lock(&sampleData, nullptr, &currentLength)))
				{
					break;
				}

				Size offset = pcmData.size();
				pcmData.resize(offset + currentLength);
				std::memcpy(pcmData.data() + offset, sampleData, currentLength);

				buffer->Unlock();
			}

			if (pcmData.empty())
			{
				SC_LOG_ERROR("AudioDecoder: mp3のデコード結果が空でした ({})", fileFsPath.string());
				return false;
			}

			/// [EN] Wrap decoded PCM samples in a standard wave container.
			/// [JP] デコードした PCM サンプルを標準的な wave コンテナへ格納する。
			Uint16 blockAlign = static_cast<Uint16>(channels * (bitsPerSample / 8));
			Uint32 bytesPerSecond = samplesPerSecond * blockAlign;
			Uint32 dataSize = static_cast<Uint32>(pcmData.size());
			Uint32 riffSize = 36 + dataSize;
			Uint32 formatSize = 16;
			Uint16 formatTag = 1;

			type = SoundType::Wave;
			data.resize(44 + pcmData.size());
			Byte* writePointer = data.data();
			std::memcpy(writePointer, "RIFF", 4); writePointer += 4;
			std::memcpy(writePointer, &riffSize, 4); writePointer += 4;
			std::memcpy(writePointer, "WAVE", 4); writePointer += 4;
			std::memcpy(writePointer, "fmt ", 4); writePointer += 4;
			std::memcpy(writePointer, &formatSize, 4); writePointer += 4;
			std::memcpy(writePointer, &formatTag, 2); writePointer += 2;
			std::memcpy(writePointer, &channels, 2); writePointer += 2;
			std::memcpy(writePointer, &samplesPerSecond, 4); writePointer += 4;
			std::memcpy(writePointer, &bytesPerSecond, 4); writePointer += 4;
			std::memcpy(writePointer, &blockAlign, 2); writePointer += 2;
			std::memcpy(writePointer, &bitsPerSample, 2); writePointer += 2;
			std::memcpy(writePointer, "data", 4); writePointer += 4;
			std::memcpy(writePointer, &dataSize, 4); writePointer += 4;
			std::memcpy(writePointer, pcmData.data(), pcmData.size());
			return true;
		}

		return false;
	}
}
