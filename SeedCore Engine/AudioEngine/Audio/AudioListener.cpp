#include <AudioEngine/Audio/AudioListener.h>
#include <AudioEngine/Audio/Audio.h>
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
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
	void AudioListener::Apply(Float deltaTime)
	{
		/// [JP] アクターの場所と前と上の向きを取る
		/// [EN] Get the actor's position and its front and up directions
		const Matrix& worldMatrix = GetActor().WorldMatrix();

		Vector3 position = worldMatrix.Translation();

		Vector3 front = Vector3::TransformDirection(Vector3::Forward, worldMatrix);
		front.Normalize();

		Vector3 up = Vector3::TransformDirection(Vector3::Up, worldMatrix);
		up.Normalize();

		/// [JP] 前のフレームからの動きで速さを出す
		/// [EN] Work out the speed from how far it moved since last frame
		Vector3 velocity = { 0.0f, 0.0f, 0.0f };
		if (deltaTime > 0.0f)
		{
			velocity = (position - previousPosition_) / deltaTime;
		}
		previousPosition_ = position;

		/// [JP] 音を聞く側に伝える
		/// [EN] Send it all to the listener
		Audio& audio = *GetActor().GetService<Audio>();
		audio.DopplerMultiplier(dopplerMultiplier_);
		audio.UpdateListener(position, front, up, velocity);
	}
}
