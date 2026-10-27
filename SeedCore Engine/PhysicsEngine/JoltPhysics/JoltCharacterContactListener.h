#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace JPH
{
	class CharacterContactListener;
}

namespace SeedCore
{
	class World;

	/**
	* [EN]
	* Bridges Jolt virtual-character contacts to SeedCore collision events
	* for the owning entity and the contacted entity. Only static and
	* kinematic solid bodies are handled here: dynamic bodies and sensors
	* already meet the character's kinematic inner body, so their events
	* come from the world's contact listener instead. Another character is
	* notified by its own listener, so a contact with one is dispatched to
	* the owning entity only. Jolt reports contacts per sub-shape of the
	* contacted body; they are merged per body, so each contacted entity
	* gets one enter, at most one stay per character update, and one exit.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt 仮想キャラクターの接触を、所有エンティティと接触先エンティティの
	* SeedCore 衝突イベントへ橋渡しする。ここで扱うのはスタティックと
	* キネマティックの実体のあるボディだけ。動的ボディとセンサーはキャラクターの
	* キネマティックな内側ボディと接触するので、それらのイベントはワールドの
	* 接触リスナーから届く。他のキャラクターへはそのキャラクター自身のリスナーが
	* 通知するので、キャラクターとの接触は所有エンティティにだけ通知する。
	* Jolt は接触を接触先ボディのサブシェイプごとに報告するが、ここではボディ
	* ごとにまとめ、接触先のエンティティごとに Enter を1回、Stay を
	* キャラクターの更新1回につき最大1回、Exit を1回通知する。
	*/
	class JoltCharacterContactListener final
	{
	public:
		JoltCharacterContactListener();
		~JoltCharacterContactListener();

		JoltCharacterContactListener(const JoltCharacterContactListener&) = delete;
		JoltCharacterContactListener& operator=(const JoltCharacterContactListener&) = delete;

	public:
		void SetTarget(World* world, EntityID entityID);

		void Begin();

	private:
		class Impl;
	
		ResourcePtr<Impl> impl_;
	};
}
