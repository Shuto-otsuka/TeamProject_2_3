#pragma once
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [JP]
	* アクターにつけると、そこが音を聞く場所になる。
	* カメラにつけることが多い。
	*
	* @brief 音を聞く場所になる部品
	*
	* [EN]
	* Put it on an actor and that spot becomes where sound is heard.
	* Usually put on the camera.
	*
	* @brief A part that marks where sound is heard
	*/
	class SEEDCORE_API AudioListener :public SeedScript
	{
	private:
		friend class AudioSystem;

	public:
		/// [JP] 動く音の高さが変わる強さ
		/// [EN] How strongly a moving sound changes pitch
		SC_REFLECTION_CLAMPED_EX("ドップラー倍率", 0.0f, 10.0f)
		Float dopplerMultiplier_ = 1.0f;

	private:
		/**
		* [JP]
		* アクターの場所と向きを、音を聞く側に伝える。
		*
		* @brief 場所と向きを伝える
		* @param deltaTime 前のフレームからの時間
		*
		* [EN]
		* Tells the listener where the actor is and which way it faces.
		*
		* @brief Sends the position and facing
		* @param deltaTime Time since the last frame
		*/
		void Apply(Float deltaTime);

	private:
		/// [JP] 前のフレームの場所。動く速さを出すのに使う
		/// [EN] Where it was last frame, used to work out how fast it moves
		Vector3 previousPosition_ = { 0.0f, 0.0f, 0.0f };
	};
	REGISTER_COMPONENT(AudioListener, "Audio");
}
