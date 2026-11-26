#pragma once
#include <AudioEngine/Audio/AudioTypes.h>

namespace SeedCore
{
	class CriManager;
	class Sound;

	/**
	* [JP]
	* 音を鳴らすための窓口。
	* プレイヤー（音を鳴らす役）と音源（音が出る場所）を作って管理する。
	*
	* @brief 音を鳴らすための窓口
	*
	* [EN]
	* The front door for playing sound.
	* It creates and keeps track of players (what plays a sound) and sources (where the sound comes from).
	*
	* @brief The front door for playing sound
	*/
	class SEEDCORE_API Audio
	{
	public:
		/**
		* [JP]
		* 音を聞く側（リスナー）を1つ作って準備する。
		*
		* @brief 音を聞く側を作って準備する
		*
		* [EN]
		* Sets up by creating the one listener that hears the sound.
		*
		* @brief Creates the listener and gets ready
		*/
		Audio();

		/**
		* [JP]
		* 作ったプレイヤーと音源とリスナーを全部片付ける。
		*
		* @brief 作ったものを全部片付ける
		*
		* [EN]
		* Cleans up every player, source and the listener it made.
		*
		* @brief Cleans up everything it made
		*/
		~Audio();

	public:
		/**
		* [JP]
		* 音を鳴らすプレイヤーを1つ作る。
		*
		* @brief  プレイヤーを作る
		* @return 作ったプレイヤー。作れなかったら nullptr
		*
		* [EN]
		* Makes one player that plays sound.
		*
		* @brief  Makes a player
		* @return The new player, or nullptr if it could not be made
		*/
		ScAudioPlayerHandle CreatePlayer();

		/**
		* [JP]
		* プレイヤーを止めてから消す。
		*
		* @brief プレイヤーを消す
		* @param player 消すプレイヤー
		*
		* [EN]
		* Stops a player and then removes it.
		*
		* @brief Removes a player
		* @param player The player to remove
		*/
		void DestroyPlayer(ScAudioPlayerHandle player);

		/**
		* [JP]
		* 音が出る場所（音源）を1つ作る。
		*
		* @brief  音源を作る
		* @return 作った音源。作れなかったら nullptr
		*
		* [EN]
		* Makes one place that sound comes from.
		*
		* @brief  Makes a source
		* @return The new source, or nullptr if it could not be made
		*/
		ScAudioSourceHandle CreateSource();

		/**
		* [JP]
		* 音源を消す。
		*
		* @brief 音源を消す
		* @param source 消す音源
		*
		* [EN]
		* Removes a source.
		*
		* @brief Removes a source
		* @param source The source to remove
		*/
		void DestroySource(ScAudioSourceHandle source);

	public:
		/**
		* [JP]
		* キューシートの中の音を、名前を指定して鳴らす。
		*
		* @brief 名前を指定してキューを鳴らす
		* @param player  鳴らすプレイヤー
		* @param sound   キューシートの音
		* @param cueName 鳴らすキューの名前
		* @param loop    くり返すなら true
		* @note  ACF が登録されていないと鳴らない
		*
		* [EN]
		* Plays a sound in a cue sheet by its name.
		*
		* @brief Plays a cue by name
		* @param player  The player to use
		* @param sound   The cue sheet sound
		* @param cueName The name of the cue to play
		* @param loop    True to repeat it
		* @note  Nothing plays unless an ACF is registered
		*/
		void PlayCue(ScAudioPlayerHandle player, Sound& sound, const String& cueName, Bool loop);

		/**
		* [JP]
		* WAV の音をそのまま鳴らす。
		*
		* @brief WAV を鳴らす
		* @param player 鳴らすプレイヤー
		* @param sound  WAV の音
		* @param loop   くり返すなら true
		*
		* [EN]
		* Plays a WAV sound as it is.
		*
		* @brief Plays a WAV
		* @param player The player to use
		* @param sound  The WAV sound
		* @param loop   True to repeat it
		*/
		void PlayWave(ScAudioPlayerHandle player, Sound& sound, Bool loop);

		/**
		* [JP]
		* 音を止める。
		*
		* @brief 音を止める
		* @param player 止めるプレイヤー
		*
		* [EN]
		* Stops the sound.
		*
		* @brief Stops the sound
		* @param player The player to stop
		*/
		void Stop(ScAudioPlayerHandle player);

		/**
		* [JP]
		* 音を一時停止する。
		*
		* @brief 一時停止する
		* @param player 一時停止するプレイヤー
		*
		* [EN]
		* Pauses the sound.
		*
		* @brief Pauses
		* @param player The player to pause
		*/
		void Pause(ScAudioPlayerHandle player);

		/**
		* [JP]
		* 一時停止した音を続きから鳴らす。
		*
		* @brief 再開する
		* @param player 再開するプレイヤー
		*
		* [EN]
		* Plays a paused sound again from where it stopped.
		*
		* @brief Resumes
		* @param player The player to resume
		*/
		void Resume(ScAudioPlayerHandle player);

	public:
		/**
		* [JP]
		* 音の大きさを変える。
		*
		* @brief 音量を変える
		* @param player 変えるプレイヤー
		* @param volume 音量。1 がそのままの大きさ
		*
		* [EN]
		* Changes how loud the sound is.
		*
		* @brief Changes the volume
		* @param player The player to change
		* @param volume The volume; 1 is the normal loudness
		*/
		void Volume(ScAudioPlayerHandle player, Float volume);

		/**
		* [JP]
		* 音の高さを変える。
		*
		* @brief 音の高さを変える
		* @param player 変えるプレイヤー
		* @param cents  高さ。100 で半音上がる
		*
		* [EN]
		* Changes how high the sound is.
		*
		* @brief Changes the pitch
		* @param player The player to change
		* @param cents  The pitch; 100 raises it by a half step
		*/
		void Pitch(ScAudioPlayerHandle player, Float cents);

	public:
		/**
		* [JP]
		* プレイヤーに音源をつなげて、音が場所から聞こえるようにする。
		*
		* @brief 音源をつなげる
		* @param player つなげるプレイヤー
		* @param source つなげる音源
		*
		* [EN]
		* Connects a source to a player so the sound comes from a place.
		*
		* @brief Connects a source
		* @param player The player to connect
		* @param source The source to connect
		*/
		void AttachSource(ScAudioPlayerHandle player, ScAudioSourceHandle source);

		/**
		* [JP]
		* 音源を外して、普通の（場所のない）音に戻す。
		*
		* @brief 音源を外す
		* @param player 外すプレイヤー
		*
		* [EN]
		* Takes the source off so it is a normal sound with no place again.
		*
		* @brief Disconnects the source
		* @param player The player to disconnect
		*/
		void DetachSource(ScAudioPlayerHandle player);

		/**
		* [JP]
		* 音源の場所と動く速さを更新する。
		*
		* @brief 音源の場所を更新する
		* @param source   更新する音源
		* @param position 音源の場所
		* @param velocity 音源の動く速さ
		*
		* [EN]
		* Updates where a source is and how fast it moves.
		*
		* @brief Updates the source position
		* @param source   The source to update
		* @param position Where the source is
		* @param velocity How fast the source moves
		*/
		void UpdateSource(ScAudioSourceHandle source, const Vector3& position, const Vector3& velocity);

		/**
		* [JP]
		* 音を聞く側の場所と向きと速さを更新する。
		*
		* @brief リスナーを更新する
		* @param position 聞く側の場所
		* @param front    前の向き
		* @param up       上の向き
		* @param velocity 動く速さ
		*
		* [EN]
		* Updates where the listener is, which way it faces and how fast it moves.
		*
		* @brief Updates the listener
		* @param position Where the listener is
		* @param front    The forward direction
		* @param up       The up direction
		* @param velocity How fast it moves
		*/
		void UpdateListener(const Vector3& position, const Vector3& front, const Vector3& up, const Vector3& velocity);

	public:
		/**
		* [JP]
		* 音が小さくなり始める距離と、聞こえなくなる距離を決める。
		*
		* @brief 聞こえる距離を決める
		* @param source      決める音源
		* @param minDistance 小さくなり始める距離
		* @param maxDistance 聞こえなくなる距離
		*
		* [EN]
		* Sets the distance where the sound starts getting quieter and where it can no longer be heard.
		*
		* @brief Sets the hearing distance
		* @param source      The source to set
		* @param minDistance Where it starts getting quieter
		* @param maxDistance Where it can no longer be heard
		*/
		void AttenuationDistance(ScAudioSourceHandle source, Float minDistance, Float maxDistance);

		/**
		* [JP]
		* 動いている音の高さが変わる強さ（ドップラー効果）を決める。
		*
		* @brief ドップラー効果の強さを決める
		* @param multiplier 強さ。0 で効果なし
		*
		* [EN]
		* Sets how strongly a moving sound changes pitch (the Doppler effect).
		*
		* @brief Sets the Doppler strength
		* @param multiplier The strength; 0 turns it off
		*/
		void DopplerMultiplier(Float multiplier);

	public:
		/**
		* [JP]
		* 今鳴っているかを調べる。
		*
		* @brief  鳴っているか調べる
		* @param  player 調べるプレイヤー
		* @return 鳴っていたら true
		*
		* [EN]
		* Checks whether it is playing right now.
		*
		* @brief  Checks if it is playing
		* @param  player The player to check
		* @return True if it is playing
		*/
		NODISCARD Bool Playing(ScAudioPlayerHandle player)const;

		/**
		* [JP]
		* 一時停止中かを調べる。
		*
		* @brief  一時停止中か調べる
		* @param  player 調べるプレイヤー
		* @return 一時停止中なら true
		*
		* [EN]
		* Checks whether it is paused.
		*
		* @brief  Checks if it is paused
		* @param  player The player to check
		* @return True if it is paused
		*/
		NODISCARD Bool Paused(ScAudioPlayerHandle player)const;

	private:
		/// [JP] 音のライブラリ（CRI）を動かしている係
		/// [EN] The part that runs the sound library (CRI)
		CriManager& criManager_;

		/// [JP] 音を聞く側
		/// [EN] The one that hears the sound
		ScAudioListenerHandle listener_ = nullptr;

		/// [JP] 作ったプレイヤーの一覧
		/// [EN] Every player that was made
		DynamicArray<ScAudioPlayerHandle> players_;

		/// [JP] 作った音源の一覧
		/// [EN] Every source that was made
		DynamicArray<ScAudioSourceHandle> sources_;
	};
}
