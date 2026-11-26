#pragma once
#include <AudioEngine/Audio/Sound.h>

namespace SeedCore
{
	/**
	* [JP]
	* 色々な形式の音のファイルを、鳴らせる形に変える。
	*
	* @brief 音のファイルを鳴らせる形に変える
	*
	* [EN]
	* Turns sound files of many formats into a form that can be played.
	*
	* @brief Turns sound files into a playable form
	*/
	class AudioDecoder
	{
	public:
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
		static Bool Decode(String filePath, SoundType& type, DynamicArray<Byte>& data, String& awbPath);
	};
}
