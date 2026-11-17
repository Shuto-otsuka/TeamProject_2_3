#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <AudioEngine/Audio/Sound.h>

namespace SeedCore
{
	/**
	* [EN]
	* Loads audio sources or their encrypted .audio caches into stable
	* Sound storage. Writing a source out as a .audio cache is private and
	* reached only by AudioResource.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 音声素材、またはその暗号化された .audio キャッシュを、安定した Sound
	* ストレージへ読み込む。素材を .audio キャッシュとして書き出す処理は
	* private で、AudioResource からだけ使う。
	*/
	class SEEDCORE_API AudioLoader :public AssetLoader<Sound>
	{
	private:
		friend class AudioResource;

	public:
		/**
		* [EN]
		* Initializes COM and Media Foundation for MP3 decoding.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* MP3 のデコードに使う COM と Media Foundation を初期化する。
		*/
		AudioLoader();

		/**
		* [EN]
		* Shuts down the facilities initialized by this loader.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このローダーが初期化した機能を終了する。
		*/
		~AudioLoader();

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
		Handle<Sound> Load(String filePath, LoadThread thread);

		/**
		* [EN]
		* Releases CRI data associated with a Sound and destroys its handle.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* Sound に関連する CRI データを解放し、ハンドルを破棄する。
		*/
		void Clear(Handle<Sound>& handle)noexcept;

	private:
		/**
		* [EN]
		* Converts a supported audio source into an encrypted .audio cache.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 対応する音声素材を暗号化された .audio キャッシュへ変換する。
		*/
		Bool Bake(String sourcePath, String cachePath);

	private:
		/// [EN] Whether this instance owns a successful COM initialization call.
		/// [JP] このインスタンスが成功した COM 初期化呼び出しを所有するか。
		Bool ownsComInitialize_ = false;

		/// [EN] Whether Media Foundation started successfully for this loader.
		/// [JP] このローダー用の Media Foundation が正常に開始されたか。
		Bool mfStarted_ = false;
	};
}
