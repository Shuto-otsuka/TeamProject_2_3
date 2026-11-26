#include <External/CRI/Include/cri_adx2le.h>
#include <AudioEngine/Audio/AudioSource.h>
#include <AudioEngine/Audio/Audio.h>
#include <AudioEngine/Audio/Sound.h>
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
	/**
	* [JP]
	* 音を鳴らすプレイヤーと音源を用意する。
	*
	* @brief プレイヤーと音源を用意する
	*
	* [EN]
	* Gets the player and source ready.
	*
	* @brief Gets the player and source ready
	*/
	void AudioSource::OnAwake()
	{
		/// [JP] プレイヤーと音源を作り、今の音量と高さを伝える
		/// [EN] Make the player and source and send the current volume and pitch
		Audio& audio = *GetActor().GetService<Audio>();

		player_ = audio.CreatePlayer();
		source_ = audio.CreateSource();

		audio.Volume(player_, volume_);
		audio.Pitch(player_, pitch_ * 100.0f);

		/// [JP] 立体音響なら音源をつなげて、聞こえる距離も決める
		/// [EN] With spatial sound, connect the source and set the hearing distance
		attached_ = spatial_;
		if (attached_)
		{
			audio.AttachSource(player_, source_);
			audio.AttenuationDistance(source_, minDistance_, maxDistance_);
		}

		previousPosition_ = GetActor().WorldMatrix().Translation();
	}

	/**
	* [JP]
	* 用意したプレイヤーと音源を片付ける。
	*
	* @brief プレイヤーと音源を片付ける
	*
	* [EN]
	* Cleans up the player and source.
	*
	* @brief Cleans up the player and source
	*/
	void AudioSource::OnDestroy()
	{
		Audio& audio = *GetActor().GetService<Audio>();

		audio.DestroyPlayer(player_);
		audio.DestroySource(source_);

		player_ = nullptr;
		source_ = nullptr;
		sound_ = nullptr;
	}

	/**
	* [JP]
	* インスペクターに、キューの選択や再生ボタンを出す。
	*
	* @brief インスペクターの表示
	*
	* [EN]
	* Shows the cue picker and play buttons in the inspector.
	*
	* @brief What the inspector shows
	*/
	void AudioSource::OnInspectorGUI()
	{
		/// [JP] 音が選ばれていない・読み込めていない・キューシートでないときは、説明だけ出す
		/// [EN] When there is no sound, it is not loaded, or it is not a cue sheet, just show a note
		if (soundID_ == 0)
		{
			ImGui::TextDisabled("サウンド未設定");
			return;
		}

		if (!sound_)
		{
			ImGui::TextDisabled("サウンド未取得");
			return;
		}

		if (sound_->Type() != SoundType::CueSheet)
		{
			ImGui::TextDisabled("波形データ（キュー指定なし）");
			return;
		}

		CriAtomExAcbHn acbHandle = sound_->AcbHandle();
		if (!acbHandle)
		{
			ImGui::TextDisabled("キューシート未取得");
			return;
		}

		/// [JP] キューシートの中のキューを一覧にして選べるようにする
		/// [EN] List the cues in the cue sheet so one can be picked
		const Char* preview = cueName_.c_str();
		if (ImGui::BeginCombo("キュー", preview ? preview : ""))
		{
			CriSint32 cueCount = criAtomExAcb_GetNumCues(acbHandle);
			for (CriSint32 cueIndex = 0; cueIndex < cueCount; ++cueIndex)
			{
				CriAtomExCueInfo cueInfo{};
				if (criAtomExAcb_GetCueInfoByIndex(acbHandle, static_cast<CriAtomExCueIndex>(cueIndex), &cueInfo) != CRI_TRUE)
				{
					continue;
				}

				if (!cueInfo.name || cueInfo.name[0] == '\0')
				{
					continue;
				}

				/// [JP] 選んだキューの名前を覚える
				/// [EN] Remember the name of the picked cue
				Bool selected = cueName_ == cueInfo.name;
				if (ImGui::Selectable(cueInfo.name, selected))
				{
					cueName_ = cueInfo.name;
				}

				if (selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}
	}

	/**
	* [JP]
	* 決めてある音を鳴らす。
	*
	* @brief 音を鳴らす
	*
	* [EN]
	* Plays the chosen sound.
	*
	* @brief Plays the sound
	*/
	void AudioSource::Play()
	{
		if (!sound_)
		{
			return;
		}

		Audio& audio = *GetActor().GetService<Audio>();

		/// [JP] キューシートならキューで、WAV ならそのまま鳴らす
		/// [EN] Play by cue for a cue sheet, or play a WAV as it is
		if (sound_->Type() == SoundType::CueSheet)
		{
			audio.PlayCue(player_, *sound_, cueName_, loop_);
			return;
		}

		audio.PlayWave(player_, *sound_, loop_);
	}

	/**
	* [JP]
	* キューの名前を指定して鳴らす。
	*
	* @brief 名前を指定して鳴らす
	* @param cueName 鳴らすキューの名前
	*
	* [EN]
	* Plays a cue by its name.
	*
	* @brief Plays a cue by name
	* @param cueName The name of the cue to play
	*/
	void AudioSource::Play(const String& cueName)
	{
		cueName_ = cueName;

		Play();
	}

	/**
	* [JP]
	* 音を止める。
	*
	* @brief 音を止める
	*
	* [EN]
	* Stops the sound.
	*
	* @brief Stops the sound
	*/
	void AudioSource::Stop()
	{
		GetActor().GetService<Audio>()->Stop(player_);
	}

	/**
	* [JP]
	* 音を一時停止する。
	*
	* @brief 一時停止する
	*
	* [EN]
	* Pauses the sound.
	*
	* @brief Pauses
	*/
	void AudioSource::Pause()
	{
		GetActor().GetService<Audio>()->Pause(player_);
	}

	/**
	* [JP]
	* 一時停止した音を続きから鳴らす。
	*
	* @brief 再開する
	*
	* [EN]
	* Plays a paused sound again.
	*
	* @brief Resumes
	*/
	void AudioSource::Resume()
	{
		GetActor().GetService<Audio>()->Resume(player_);
	}

	/**
	* [JP]
	* 今鳴っているかを調べる。
	*
	* @brief  鳴っているか調べる
	* @return 鳴っていたら true
	*
	* [EN]
	* Checks whether it is playing.
	*
	* @brief  Checks if it is playing
	* @return True if it is playing
	*/
	Bool AudioSource::Playing()const
	{
		return GetActor().GetService<Audio>()->Playing(player_);
	}

	/**
	* [JP]
	* 読み込んだ音を受け取って、鳴らす準備をする。
	*
	* @brief 鳴らす準備をする
	* @param sound 使う音。音がないときは nullptr
	*
	* [EN]
	* Takes the loaded sound and gets ready to play it.
	*
	* @brief Gets ready to play
	* @param sound The sound to use, or nullptr if there is none
	*/
	void AudioSource::Build(Sound* sound)
	{
		/// [JP] 違う音に変わったら、今の音を止めて自動再生をやり直す
		/// [EN] If the sound changed, stop the current one and allow auto play again
		if (player_ && sound_ != sound)
		{
			GetActor().GetService<Audio>()->Stop(player_);
			autoPlayStarted_ = false;
		}

		sound_ = sound;
		resolvedSoundID_ = soundID_;
	}

	/**
	* [JP]
	* 毎フレーム、音源の場所や音量を伝える。
	*
	* @brief 場所や音量を伝える
	* @param deltaTime 前のフレームからの時間
	*
	* [EN]
	* Every frame, sends the source position, volume and so on.
	*
	* @brief Sends position and volume
	* @param deltaTime Time since the last frame
	*/
	void AudioSource::Apply(Float deltaTime)
	{
		Audio& audio = *GetActor().GetService<Audio>();

		/// [JP] 前のフレームからの動きで速さを出す
		/// [EN] Work out the speed from how far it moved since last frame
		Vector3 position = GetActor().WorldMatrix().Translation();
		Vector3 velocity = { 0.0f, 0.0f, 0.0f };
		if (deltaTime > 0.0f)
		{
			velocity = (position - previousPosition_) / deltaTime;
		}
		previousPosition_ = position;

		/// [JP] 立体音響の設定が変わったら、音源をつなげ直すか外す
		/// [EN] If the spatial setting changed, connect or disconnect the source
		if (attached_ != spatial_)
		{
			attached_ = spatial_;

			if (attached_)
			{
				audio.AttachSource(player_, source_);
			}
			else
			{
				audio.DetachSource(player_);
			}
		}

		/// [JP] 音源の場所・音量・高さを伝える
		/// [EN] Send the source position, volume and pitch
		if (attached_)
		{
			audio.AttenuationDistance(source_, minDistance_, maxDistance_);
			audio.UpdateSource(source_, position, velocity);
		}

		audio.Volume(player_, volume_);
		audio.Pitch(player_, pitch_ * 100.0f);

		/// [JP] 自動再生がまだなら、ここで1回だけ鳴らす
		/// [EN] If auto play has not happened yet, play once here
		if (autoPlay_ && !autoPlayStarted_ && sound_)
		{
			autoPlayStarted_ = true;
			Play();
		}
	}

	/**
	* [JP]
	* 音の準備がまだ終わっていないかを調べる。
	*
	* @brief  準備待ちか調べる
	* @return まだ準備が必要なら true
	*
	* [EN]
	* Checks whether the sound still needs to be set up.
	*
	* @brief  Checks if setup is still needed
	* @return True if it still needs setting up
	*/
	Bool AudioSource::Pending()const
	{
		return soundID_ != resolvedSoundID_;
	}
}
