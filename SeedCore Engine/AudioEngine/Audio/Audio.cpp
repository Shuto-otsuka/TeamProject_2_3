#include <External/CRI/Include/cri_adx2le.h>
#include <AudioEngine/Audio/Audio.h>
#include <AudioEngine/Audio/Sound.h>
#include <AudioEngine/CRI/CriManager.h>
#include <FoundationEngine/Resource/Gateway.h>
#include <FoundationEngine/Log/Warning.h>

static_assert(std::is_same_v<SeedCore::ScAudioPlayerHandle, CriAtomExPlayerHn>);
static_assert(std::is_same_v<SeedCore::ScAudioSourceHandle, CriAtomEx3dSourceHn>);
static_assert(std::is_same_v<SeedCore::ScAudioListenerHandle, CriAtomEx3dListenerHn>);
static_assert(std::is_same_v<SeedCore::ScAudioBankHandle, CriAtomExAcbHn>);

namespace SeedCore
{
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
	Audio::Audio() :criManager_(Gateway::GetCriManager())
	{
		listener_ = criAtomEx3dListener_Create(nullptr, nullptr, 0);
	}

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
	Audio::~Audio()
	{
		/// [JP] 音のライブラリがもう止まっていたら、手元の記録だけ消して終わる
		/// [EN] If the sound library is already stopped, just forget what we kept and finish
		if (!criAtomEx_IsInitialized())
		{
			players_.clear();
			sources_.clear();
			listener_ = nullptr;
			return;
		}

		/// [JP] プレイヤーを止めてから消す
		/// [EN] Stop each player, then remove it
		for (CriAtomExPlayerHn player : players_)
		{
			criAtomExPlayer_Stop(player);
			criAtomExPlayer_Destroy(player);
		}
		players_.clear();

		/// [JP] 音源とリスナーも消す
		/// [EN] Remove the sources and the listener too
		for (CriAtomEx3dSourceHn source : sources_)
		{
			criAtomEx3dSource_Destroy(source);
		}
		sources_.clear();

		if (listener_)
		{
			criAtomEx3dListener_Destroy(listener_);
			listener_ = nullptr;
		}
	}

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
	CriAtomExPlayerHn Audio::CreatePlayer()
	{
		CriAtomExPlayerHn player = criAtomExPlayer_Create(nullptr, nullptr, 0);
		if (!player)
		{
			return nullptr;
		}

		players_.push_back(player);
		return player;
	}

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
	void Audio::DestroyPlayer(CriAtomExPlayerHn player)
	{
		if (!player)
		{
			return;
		}

		erase(players_, player);

		criAtomExPlayer_Stop(player);
		criAtomExPlayer_Destroy(player);
	}

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
	CriAtomEx3dSourceHn Audio::CreateSource()
	{
		CriAtomEx3dSourceHn source = criAtomEx3dSource_Create(nullptr, nullptr, 0);
		if (!source)
		{
			return nullptr;
		}

		sources_.push_back(source);
		return source;
	}

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
	void Audio::DestroySource(CriAtomEx3dSourceHn source)
	{
		if (!source)
		{
			return;
		}

		erase(sources_, source);

		criAtomEx3dSource_Destroy(source);
	}

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
	void Audio::PlayCue(CriAtomExPlayerHn player, Sound& sound, const String& cueName, Bool loop)
	{
		/// [JP] プレイヤーとキューシートと名前がそろっていなければ鳴らさない
		/// [EN] Do nothing unless there is a player, a cue sheet and a name
		if (!player || sound.Type() != SoundType::CueSheet || cueName.view().empty())
		{
			return;
		}

		/// [JP] ACF が登録されていないとキューシートは鳴らせない
		/// [EN] A cue sheet cannot play unless an ACF is registered
		CriAtomExAcfInfo acfInfo{};
		if (criAtomExAcf_GetAcfInfo(&acfInfo) != CRI_TRUE)
		{
			SC_LOG_WARNING("Audio: ACFが登録されていないため、キューシートを再生できません。EditorConfig の acfPath に .acf を指定してください");
			return;
		}

		if (!sound.AcbHandle())
		{
			SC_LOG_WARNING("Audio: ACBがCRIへ登録されていないため、キューシートを再生できません");
			return;
		}

		/// [JP] くり返しの設定をしてから、名前でキューを選んで鳴らす
		/// [EN] Set up looping, pick the cue by name and play it
		criAtomExPlayer_LimitLoopCount(player, loop ? CRIATOMEXPLAYER_NO_LOOP_LIMITATION : CRIATOMEXPLAYER_IGNORE_LOOP);

		criAtomExPlayer_SetDataRequestCallback(player, nullptr, nullptr);

		criAtomExPlayer_SetCueName(player, sound.AcbHandle(), cueName.c_str());
		criAtomExPlayer_Start(player);
	}

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
	void Audio::PlayWave(CriAtomExPlayerHn player, Sound& sound, Bool loop)
	{
		/// [JP] WAV のデータとして正しい形かを確かめる
		/// [EN] Make sure the data really is a WAV
		if (!player || sound.Type() != SoundType::Wave || !sound.Data() || sound.DataSize() < 44)
		{
			return;
		}

		const Byte* bytes = static_cast<const Byte*>(sound.Data());
		if (std::memcmp(bytes, "RIFF", 4) != 0 || std::memcmp(bytes + 8, "WAVE", 4) != 0)
		{
			return;
		}

		/// [JP] fmt の部分を探して、チャンネル数とサンプリングレートを読む
		/// [EN] Find the fmt part and read the channel count and sample rate
		Uint16 channels = 0;
		Uint32 samplingRate = 0;
		Size offset = 12;
		while (offset + 8 <= sound.DataSize())
		{
			Uint32 chunkSize = 0;
			std::memcpy(&chunkSize, bytes + offset + 4, 4);

			if (std::memcmp(bytes + offset, "fmt ", 4) == 0 && chunkSize >= 16)
			{
				std::memcpy(&channels, bytes + offset + 10, 2);
				std::memcpy(&samplingRate, bytes + offset + 12, 4);
				break;
			}

			offset += 8 + static_cast<Size>(chunkSize) + (chunkSize & 1);
		}

		if (channels == 0 || samplingRate == 0)
		{
			return;
		}

		/// [JP] 読んだ情報をプレイヤーに渡す
		/// [EN] Hand what we read to the player
		criAtomExPlayer_SetData(player, sound.Data(), static_cast<CriSint32>(sound.DataSize()));
		criAtomExPlayer_SetFormat(player, CRIATOMEX_FORMAT_WAVE);
		criAtomExPlayer_SetNumChannels(player, static_cast<CriSint32>(channels));
		criAtomExPlayer_SetSamplingRate(player, static_cast<CriSint32>(samplingRate));

		/// [JP] くり返すときは、終わったら同じデータをもう一度流す
		/// [EN] When looping, feed the same data again once it ends
		criAtomExPlayer_SetDataRequestCallback(player, loop ? static_cast<CriAtomExPlayerDataRequestCbFunc>([](void* obj, CriAtomExPlaybackId playbackID, CriAtomPlayerHn atomPlayer) { criAtomPlayer_SetPreviousDataAgain(atomPlayer); }) : nullptr, nullptr);

		criAtomExPlayer_Start(player);
	}

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
	void Audio::Stop(CriAtomExPlayerHn player)
	{
		if (!player)
		{
			return;
		}

		criAtomExPlayer_Stop(player);
	}

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
	void Audio::Pause(CriAtomExPlayerHn player)
	{
		if (!player)
		{
			return;
		}

		criAtomExPlayer_Pause(player, CRI_TRUE);
	}

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
	void Audio::Resume(CriAtomExPlayerHn player)
	{
		if (!player)
		{
			return;
		}

		criAtomExPlayer_Resume(player, CRIATOMEX_RESUME_ALL_PLAYBACK);
	}

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
	void Audio::Volume(CriAtomExPlayerHn player, Float volume)
	{
		if (!player)
		{
			return;
		}

		criAtomExPlayer_SetVolume(player, volume);
		criAtomExPlayer_UpdateAll(player);
	}

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
	void Audio::Pitch(CriAtomExPlayerHn player, Float cents)
	{
		if (!player)
		{
			return;
		}

		criAtomExPlayer_SetPitch(player, cents);
		criAtomExPlayer_UpdateAll(player);
	}

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
	void Audio::AttachSource(CriAtomExPlayerHn player, CriAtomEx3dSourceHn source)
	{
		if (!player || !source)
		{
			return;
		}

		criAtomExPlayer_Set3dSourceHn(player, source);
		criAtomExPlayer_Set3dListenerHn(player, listener_);

		criAtomExPlayer_SetPanType(player, CRIATOMEX_PAN_TYPE_3D_POS);
		criAtomExPlayer_UpdateAll(player);
	}

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
	void Audio::DetachSource(CriAtomExPlayerHn player)
	{
		if (!player)
		{
			return;
		}

		criAtomExPlayer_Set3dSourceHn(player, nullptr);
		criAtomExPlayer_Set3dListenerHn(player, nullptr);

		criAtomExPlayer_SetPanType(player, CRIATOMEX_PAN_TYPE_AUTO);
		criAtomExPlayer_UpdateAll(player);
	}

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
	void Audio::UpdateSource(CriAtomEx3dSourceHn source, const Vector3& position, const Vector3& velocity)
	{
		if (!source)
		{
			return;
		}

		/// [JP] エンジンの形から音のライブラリの形に写す
		/// [EN] Copy from the engine's format into the sound library's format
		CriAtomExVector sourcePosition{};
		sourcePosition.x = position.x;
		sourcePosition.y = position.y;
		sourcePosition.z = position.z;

		CriAtomExVector sourceVelocity{};
		sourceVelocity.x = velocity.x;
		sourceVelocity.y = velocity.y;
		sourceVelocity.z = velocity.z;

		criAtomEx3dSource_SetPosition(source, &sourcePosition);
		criAtomEx3dSource_SetVelocity(source, &sourceVelocity);
		criAtomEx3dSource_Update(source);
	}

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
	void Audio::UpdateListener(const Vector3& position, const Vector3& front, const Vector3& up, const Vector3& velocity)
	{
		if (!listener_)
		{
			return;
		}

		/// [JP] 場所・前・上・速さを音のライブラリの形に写す
		/// [EN] Copy position, front, up and velocity into the sound library's format
		CriAtomExVector listenerPosition{};
		listenerPosition.x = position.x;
		listenerPosition.y = position.y;
		listenerPosition.z = position.z;

		CriAtomExVector listenerFront{};
		listenerFront.x = front.x;
		listenerFront.y = front.y;
		listenerFront.z = front.z;

		CriAtomExVector listenerUp{};
		listenerUp.x = up.x;
		listenerUp.y = up.y;
		listenerUp.z = up.z;

		CriAtomExVector listenerVelocity{};
		listenerVelocity.x = velocity.x;
		listenerVelocity.y = velocity.y;
		listenerVelocity.z = velocity.z;

		/// [JP] 写した値をリスナーに渡して更新する
		/// [EN] Hand the values to the listener and update it
		criAtomEx3dListener_SetPosition(listener_, &listenerPosition);
		criAtomEx3dListener_SetOrientation(listener_, &listenerFront, &listenerUp);
		criAtomEx3dListener_SetVelocity(listener_, &listenerVelocity);
		criAtomEx3dListener_Update(listener_);
	}

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
	void Audio::AttenuationDistance(CriAtomEx3dSourceHn source, Float minDistance, Float maxDistance)
	{
		if (!source)
		{
			return;
		}

		criAtomEx3dSource_SetMinMaxAttenuationDistance(source, minDistance, maxDistance);
		criAtomEx3dSource_Update(source);
	}

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
	void Audio::DopplerMultiplier(Float multiplier)
	{
		if (!listener_)
		{
			return;
		}

		criAtomEx3dListener_SetDopplerMultiplier(listener_, multiplier);
		criAtomEx3dListener_Update(listener_);
	}

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
	Bool Audio::Playing(CriAtomExPlayerHn player)const
	{
		if (!player)
		{
			return false;
		}

		return criAtomExPlayer_GetStatus(player) == CRIATOMEXPLAYER_STATUS_PLAYING;
	}

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
	Bool Audio::Paused(CriAtomExPlayerHn player)const
	{
		if (!player)
		{
			return false;
		}

		return criAtomExPlayer_IsPaused(player) == CRI_TRUE;
	}
}
