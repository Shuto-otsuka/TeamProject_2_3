#include <AudioEngine/Audio/AudioSystem.h>
#include <AudioEngine/Audio/AudioSource.h>
#include <AudioEngine/Audio/AudioListener.h>
#include <AudioEngine/Audio/AudioResource.h>
#include <AudioEngine/Audio/Sound.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/ActorAccessor.h>
#include <FoundationEngine/World/ECS/Query/Query.h>

namespace SeedCore
{
	/**
	* [JP]
	* 音の準備が終わっていない部品に、読み込んだ音を渡す。
	*
	* @brief 部品に音を渡す
	* @param audioResource 音を探す場所
	* @param world         部品を探す世界
	*
	* [EN]
	* Hands loaded sounds to the parts that are not set up yet.
	*
	* @brief Hands sounds to the parts
	* @param audioResource Where to look up sounds
	* @param world         The world to look for parts in
	*/
	void AudioSystem::ResolveSound(AudioResource& audioResource, World& world)
	{
		/// [JP] 音を鳴らす部品を持つアクターを全部集める
		/// [EN] Gather every actor that has a sound source part
		DynamicArray<EntityID> audioSourceEntities;
		Query<Read<AudioSource>>(world).ForEach([&](EntityID entityID, const AudioSource&) { audioSourceEntities.push_back(entityID); });
		for (EntityID id : audioSourceEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor)
			{
				continue;
			}

			/// [JP] もう準備が終わっている部品は飛ばす
			/// [EN] Skip parts that are already set up
			AudioSource* audioSource = world.GetComponent<AudioSource>(ActorAccessor::GetEntity(actor));
			if (!audioSource || !audioSource->Pending())
			{
				continue;
			}

			/// [JP] 音が選ばれていなければ、音なしで準備する
			/// [EN] With no sound chosen, set it up with no sound
			if (audioSource->soundID_ == 0)
			{
				audioSource->Build(nullptr);
				continue;
			}

			/// [JP] 音がまだ読み込めていなければ、次のフレームでまた試す
			/// [EN] If the sound is not loaded yet, try again next frame
			Handle<Sound> handle = audioResource.GetHandle(audioSource->soundID_);
			if (handle.empty())
			{
				continue;
			}

			Sound* sound = audioResource.Resolve(handle);
			if (!sound)
			{
				continue;
			}

			audioSource->Build(sound);
		}
	}

	/**
	* [JP]
	* 聞く側と鳴らす側の部品を、毎フレーム更新する。
	*
	* @brief 部品を更新する
	* @param world     部品を探す世界
	* @param deltaTime 前のフレームからの時間
	*
	* [EN]
	* Updates the listener and source parts every frame.
	*
	* @brief Updates the parts
	* @param world     The world to look for parts in
	* @param deltaTime Time since the last frame
	*/
	void AudioSystem::Update(World& world, Float deltaTime)
	{
		/// [JP] まず音を聞く側を更新する
		/// [EN] First update the listeners
		DynamicArray<EntityID> audioListenerEntities;
		Query<Read<AudioListener>>(world).ForEach([&](EntityID entityID, const AudioListener&) { audioListenerEntities.push_back(entityID); });
		for (EntityID id : audioListenerEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor || !actor.Active())
			{
				continue;
			}

			AudioListener* audioListener = world.GetComponent<AudioListener>(ActorAccessor::GetEntity(actor));
			if (!audioListener)
			{
				continue;
			}

			audioListener->Apply(deltaTime);
		}

		/// [JP] 次に音を鳴らす側を更新する
		/// [EN] Then update the sound sources
		DynamicArray<EntityID> audioSourceEntities;
		Query<Read<AudioSource>>(world).ForEach([&](EntityID entityID, const AudioSource&) { audioSourceEntities.push_back(entityID); });
		for (EntityID id : audioSourceEntities)
		{
			Actor actor = WorldAccessor::Make(world, id);
			if (!actor)
			{
				continue;
			}

			AudioSource* audioSource = world.GetComponent<AudioSource>(ActorAccessor::GetEntity(actor));
			if (!audioSource)
			{
				continue;
			}

			/// [JP] 動いていないアクターの音は止める
			/// [EN] Stop the sound of actors that are not active
			if (!actor.Active())
			{
				if (audioSource->Playing())
				{
					audioSource->Stop();
				}
				continue;
			}

			audioSource->Apply(deltaTime);
		}
	}
}
