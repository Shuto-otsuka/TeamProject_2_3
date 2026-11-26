#include <AudioEngine/Audio/AudioDecoder.h>
#include <FoundationEngine/File/FileUtility.h>
#include <FoundationEngine/Serialization/Encryption/Aes256.h>
#include <FoundationEngine/Serialization/Encryption/Sha256.h>
#include <FoundationEngine/Log/Error.h>

namespace SeedCore
{
	/**
	* [JP]
	* ファイルを読んで、鳴らせるデータにする。
	*
	* @brief  ファイルを鳴らせるデータにする
	* @param  filePath 読むファイル
	* @param  type     読んだ音の種類が入る
	* @param  data     読んだデータが入る
	* @param  awbPath  一緒に使う AWB ファイルの場所が入る
	* @return うまくいったら true
	*
	* [EN]
	* Reads a file and turns it into playable data.
	*
	* @brief  Turns a file into playable data
	* @param  filePath The file to read
	* @param  type     Gets the kind of sound read
	* @param  data     Gets the data read
	* @param  awbPath  Gets where the matching AWB file is
	* @return True if it worked
	*/
	Bool AudioDecoder::Decode(String filePath, SoundType& type, DynamicArray<Byte>& data, String& awbPath)
	{
		/// [JP] 拡張子を小文字にして、それで読み方を決める
		/// [EN] Lower-case the extension and pick how to read from it
		std::filesystem::path fileFsPath(filePath.c_str());
		std::string extension = fileFsPath.extension().string();
		std::ranges::transform(extension, extension.begin(), [](Uchar c) { return static_cast<Char>(std::tolower(c)); });

		awbPath = String();

		/// [JP] .audio はエンジンが作った暗号化済みのファイル
		/// [EN] An .audio file is an encrypted file the engine made
		if (extension == ".audio")
		{
			std::ifstream ifs(fileFsPath, std::ios::binary);
			if (!ifs)
			{
				SC_LOG_ERROR("AudioDecoder: .audioを開けませんでした ({})", fileFsPath.string());
				return false;
			}

			/// [JP] 見出しを読んで、正しいファイルかを確かめる
			/// [EN] Read the header and check it is a valid file
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

			/// [JP] 中身の一覧から、本体と AWB の場所を探す
			/// [EN] Look through the parts list for the main data and the AWB
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

			/// [JP] 本体を読んで復号する
			/// [EN] Read the main data and decrypt it
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

			/// [JP] AWB があれば、このファイルの中から読むための場所を作る
			/// [EN] If there is an AWB, make a path that reads it from inside this file
			if (hasStreamBlob)
			{
				awbPath = String("seedcore_audio://" + std::filesystem::absolute(fileFsPath).generic_string());
			}
			return true;
		}

		/// [JP] .acb と .wav はそのまま読む
		/// [EN] Read .acb and .wav files as they are
		else if (extension == ".acb" || extension == ".wav")
		{
			type = (extension == ".acb") ? SoundType::CueSheet : SoundType::Wave;

			DynamicArray<Uint8> sourceBytes = FileUtility::LoadFileBinary(String(fileFsPath.string()));
			if (sourceBytes.empty())
			{
				SC_LOG_ERROR("AudioDecoder: 音声ファイルを読み込めませんでした ({})", fileFsPath.string());
				return false;
			}
			data.resize(sourceBytes.size());
			std::memcpy(data.data(), sourceBytes.data(), sourceBytes.size());

			/// [JP] .acb の横に .awb があれば一緒に使う
			/// [EN] If there is an .awb next to the .acb, use it too
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

		/// [JP] .mp3 は Media Foundation で PCM に変換する
		/// [EN] Convert an .mp3 to PCM with Media Foundation
		else if (extension == ".mp3")
		{
			std::wstring widePath = fileFsPath.wstring();
			Microsoft::WRL::ComPtr<IMFSourceReader> sourceReader;
			if (FAILED(MFCreateSourceReaderFromURL(widePath.c_str(), nullptr, sourceReader.ReleaseAndGetAddressOf())))
			{
				SC_LOG_ERROR("AudioDecoder: mp3を開けませんでした ({})", fileFsPath.string());
				return false;
			}

			sourceReader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
			sourceReader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), TRUE);

			/// [JP] 出力を PCM にするよう設定する
			/// [EN] Ask for PCM output
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

			/// [JP] チャンネル数などの形を読む
			/// [EN] Read the format, such as the channel count
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

			/// [JP] 最後まで少しずつ読んでつなげる
			/// [EN] Read it bit by bit to the end and join it together
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

			/// [JP] WAV の見出しを作って、PCM の前につける
			/// [EN] Build a WAV header and put it in front of the PCM data
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
