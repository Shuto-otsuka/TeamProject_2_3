#pragma once
#include <AudioEngine/Audio/Sound.h>

namespace SeedCore
{
	/**
	* [EN]
	* Turns an audio file into the plain bytes a Sound holds. A ".audio"
	* cache is decrypted; a source (".wav"/".mp3"/".acb") is converted in
	* memory into the same bytes its cache would hold.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 音声ファイルを、Sound が持つ平文のバイト列にする。".audio"
	* キャッシュは復号し、素材（".wav"/".mp3"/".acb"）はそのキャッシュが
	* 持つのと同じバイト列へメモリ上で変換する。
	*/
	class AudioDecoder
	{
	public:
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
		static Bool Decode(String filePath, SoundType& type, DynamicArray<Byte>& data, String& awbPath);
	};
}
