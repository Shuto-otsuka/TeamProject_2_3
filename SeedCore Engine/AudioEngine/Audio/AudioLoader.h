#pragma once
#include <FoundationEngine/Resource/Asset/AssetLoader.h>
#include <AudioEngine/Audio/Sound.h>

namespace SeedCore
{
	/**
	* [EN]
	* Loads encrypted audio caches into stable Sound storage. Converting a
	* supported source file into the engine's .audio cache format is
	* private and reached only by AudioResource.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 暗号化された音声キャッシュを安定した Sound ストレージへ読み込む。
	* 対応する素材ファイルをエンジンの .audio キャッシュ形式へ変換する処理は
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
		* Loads a Sound from the .audio cache at filePath, or from the one
		* beside the source file filePath names. A source plays only through
		* its cache, which AudioResource::Generate writes; nothing is written
		* here. Returns a null handle on failure. thread is unused: a Sound
		* needs no per-thread state.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* filePath の .audio キャッシュ、または filePath が示す素材ファイルの
		* 隣の .audio キャッシュから Sound を読み込む。素材はキャッシュを通して
		* しか再生できず、キャッシュは AudioResource::Generate が書く。ここでは
		* 何も書かない。失敗時は null ハンドルを返す。thread は使わない（Sound は
		* スレッドごとの状態を持たないため）。
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
