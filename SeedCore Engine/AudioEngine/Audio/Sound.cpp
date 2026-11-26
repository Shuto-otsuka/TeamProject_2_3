#include <AudioEngine/Audio/Sound.h>

namespace SeedCore
{
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
	SoundType Sound::Type()const
	{
		return type_;
	}

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
	ScAudioBankHandle Sound::AcbHandle()const
	{
		return acbHandle_;
	}

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
	void* Sound::Data()
	{
		return data_.data();
	}

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
	Size Sound::DataSize()const
	{
		return data_.size();
	}

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
	const String& Sound::AwbPath()const
	{
		return awbPath_;
	}
}
