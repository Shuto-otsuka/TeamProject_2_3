#pragma once
#include <AudioEngine/Audio/AudioTypes.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	class Sound;

	/**
	* [JP]
	* アクターにつけると、そこから音が鳴る部品。
	*
	* @brief 音を鳴らす部品
	*
	* [EN]
	* Put it on an actor and sound plays from there.
	*
	* @brief A part that plays sound
	*/
	class AudioSource :public SeedScript
	{
	private:
		friend class AudioSystem;

	public:
		/// [JP] 鳴らす音のアセット番号
		/// [EN] The asset number of the sound to play
		SC_PAYLOAD_FIELD_EX("サウンド", Audio)
		Uint32 soundID_ = 0;

		/// [JP] キューシートのときに鳴らすキューの名前
		/// [EN] The cue name to play for a cue sheet
		SC_SERIALIZE_FIELD()
		String cueName_;

		/// [JP] 始まったときに自動で鳴らすか
		/// [EN] Whether it plays on its own when it starts
		SC_REFLECTION_FIELD_EX("自動再生")
		Bool autoPlay_ = false;

		/// [JP] くり返し鳴らすか
		/// [EN] Whether it repeats
		SC_REFLECTION_FIELD_EX("ループ再生")
		Bool loop_ = false;

		/// [JP] 音の大きさ
		/// [EN] How loud it is
		SC_REFLECTION_CLAMPED_EX("音量", 0.0f, 5.0f)
		Float volume_ = 1.0f;

		/// [JP] 音の高さ（半音いくつ分）
		/// [EN] The pitch, in half steps
		SC_REFLECTION_CLAMPED_EX("ピッチ(半音)", -12.0f, 12.0f)
		Float pitch_ = 0.0f;

		/// [JP] 場所によって聞こえ方を変えるか
		/// [EN] Whether it sounds different depending on where it is
		SC_REFLECTION_FIELD_EX("立体音響")
		Bool spatial_ = true;

		/// [JP] 音が小さくなり始める距離
		/// [EN] Where the sound starts getting quieter
		SC_REFLECTION_FIELD_CONDITION(spatial_)
		SC_REFLECTION_CLAMPED_EX("減衰開始距離", 0.0f, 10000.0f)
		Float minDistance_ = 1.0f;

		/// [JP] 音が聞こえなくなる距離
		/// [EN] Where the sound can no longer be heard
		SC_REFLECTION_FIELD_CONDITION(spatial_)
		SC_REFLECTION_CLAMPED_EX("減衰終了距離", 0.0f, 10000.0f)
		Float maxDistance_ = 50.0f;

	public:
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
		SEEDCORE_API void OnAwake();

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
		SEEDCORE_API void OnDestroy();

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
		SEEDCORE_API void OnInspectorGUI();

	public:
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
		SEEDCORE_API void Play();

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
		SEEDCORE_API void Play(const String& cueName);

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
		SEEDCORE_API void Stop();

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
		SEEDCORE_API void Pause();

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
		SEEDCORE_API void Resume();

	public:
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
		SEEDCORE_API Bool Playing()const;

	private:
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
		void Build(Sound* sound);

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
		void Apply(Float deltaTime);

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
		Bool Pending()const;

	private:
		/// [JP] 音を鳴らすプレイヤー
		/// [EN] The player that plays the sound
		ScAudioPlayerHandle player_ = nullptr;

		/// [JP] 音が出る場所
		/// [EN] Where the sound comes from
		ScAudioSourceHandle source_ = nullptr;

		/// [JP] 使っている音
		/// [EN] The sound in use
		Sound* sound_ = nullptr;

		/// [JP] 前のフレームの場所。動く速さを出すのに使う
		/// [EN] Where it was last frame, used to work out how fast it moves
		Vector3 previousPosition_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 準備に使った音の番号。変わったら準備し直す
		/// [EN] The sound number it was set up with; set up again if it changes
		Uint32 resolvedSoundID_ = 0;

		/// [JP] 音源をプレイヤーにつないでいるか
		/// [EN] Whether the source is connected to the player
		Bool attached_ = false;

		/// [JP] 自動再生をもう済ませたか
		/// [EN] Whether the auto play has already happened
		Bool autoPlayStarted_ = false;
	};
	REGISTER_COMPONENT(AudioSource, "Audio");
}
