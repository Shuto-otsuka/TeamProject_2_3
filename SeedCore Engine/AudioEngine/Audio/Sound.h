#pragma once
#include <AudioEngine/Audio/AudioTypes.h>

namespace SeedCore
{
	/**
	* [JP]
	* 音のデータの種類。
	*
	* @brief 音のデータの種類
	*
	* [EN]
	* What kind of sound data it is.
	*
	* @brief The kind of sound data
	*/
	enum class SoundType :Uint32
	{
		/// [JP] キューシート（いくつもの音をまとめたもの）
		/// [EN] A cue sheet (many sounds packed together)
		CueSheet,

		/// [JP] WAV の音1つ
		/// [EN] A single WAV sound
		Wave,
	};

	/**
	* [JP]
	* 読み込んだ音のデータ。
	*
	* @brief 読み込んだ音
	*
	* [EN]
	* The data of a loaded sound.
	*
	* @brief A loaded sound
	*/
	class SEEDCORE_API Sound :public NonCopyable
	{
	private:
		friend class AudioLoader;

	public:
		Sound() = default;

		~Sound() = default;

		Sound(Sound&&)noexcept = default;

		Sound& operator=(Sound&&)noexcept = default;

	public:
		/**
		* [JP]
		* 音の種類を返す。
		*
		* @brief  音の種類を返す
		* @return 音の種類
		*
		* [EN]
		* Returns the kind of sound.
		*
		* @brief  Returns the kind
		* @return The kind of sound
		*/
		NODISCARD SoundType Type()const;

		/**
		* [JP]
		* キューシートの目印を返す。
		*
		* @brief  キューシートを返す
		* @return キューシートの目印。WAV なら nullptr
		*
		* [EN]
		* Returns the cue sheet handle.
		*
		* @brief  Returns the cue sheet
		* @return The cue sheet handle, or nullptr for a WAV
		*/
		NODISCARD ScAudioBankHandle AcbHandle()const;

		/**
		* [JP]
		* 音のデータの先頭を返す。
		*
		* @brief  データを返す
		* @return データの先頭
		*
		* [EN]
		* Returns the start of the sound data.
		*
		* @brief  Returns the data
		* @return The start of the data
		*/
		NODISCARD void* Data();

		/**
		* [JP]
		* 音のデータの大きさを返す。
		*
		* @brief  データの大きさを返す
		* @return データのバイト数
		*
		* [EN]
		* Returns the size of the sound data.
		*
		* @brief  Returns the data size
		* @return The data size in bytes
		*/
		NODISCARD Size DataSize()const;

		/**
		* [JP]
		* 一緒に使う AWB ファイルの場所を返す。
		*
		* @brief  AWB の場所を返す
		* @return AWB ファイルの場所
		*
		* [EN]
		* Returns where the matching AWB file is.
		*
		* @brief  Returns the AWB path
		* @return Where the AWB file is
		*/
		NODISCARD const String& AwbPath()const;

	private:
		/// [JP] 音の種類
		/// [EN] The kind of sound
		SoundType type_ = SoundType::Wave;

		/// [JP] キューシートの目印
		/// [EN] The cue sheet handle
		ScAudioBankHandle acbHandle_ = nullptr;

		/// [JP] 音のデータ
		/// [EN] The sound data
		DynamicArray<Byte> data_;

		/// [JP] 一緒に使う AWB ファイルの場所
		/// [EN] Where the matching AWB file is
		String awbPath_;
	};
}
