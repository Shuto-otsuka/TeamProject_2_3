#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <External/JoltPhysics/Jolt/Physics/Character/CharacterVirtual.h>

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
	class JoltCharacterContactListener final :public JPH::CharacterContactListener, public JPH::RefTarget<JoltCharacterContactListener>
	{
	public:
		JPH_OVERRIDE_NEW_DELETE

	public:
		/**
		* [EN]
		* Sets the World and entity that receive this listener's contact events.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* このリスナーの接触イベントを受け取る World とエンティティを設定する。
		*/
		void SetTarget(World* world, EntityID entityID);

		/**
		* [EN]
		* Starts a new character update. Each contacted body dispatches at
		* most one enter or stay until the next call. Call it right before
		* the character's update.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キャラクターの新しい更新を始める。次に呼ぶまで、接触先ボディごとの
		* Enter か Stay は最大1回になる。キャラクターの更新の直前に呼ぶ。
		*/
		void Begin();

	public:
		/**
		* [EN]
		* Dispatches an enter event when the first sub-shape of a body is
		* touched, and a stay event when another one of a body already in
		* contact is touched. Dynamic bodies and sensors are ignored, as
		* the inner body reports them.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディの最初のサブシェイプに触れたときに Enter イベントを、既に接触中の
		* ボディの別のサブシェイプに触れたときに Stay イベントを通知する。
		* 動的ボディとセンサーは内側ボディが知らせるので無視する。
		*/
		void OnContactAdded(const JPH::CharacterVirtual* character, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& settings)override;

		/**
		* [EN]
		* Dispatches a stay event for a body still in contact, at most once
		* per character update.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 接触が続いているボディの Stay イベントを、キャラクターの更新1回につき
		* 最大1回通知する。
		*/
		void OnContactPersisted(const JPH::CharacterVirtual* character, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& settings)override;

		/**
		* [EN]
		* Dispatches an exit event when the last touched sub-shape of a body
		* is released, even if the body has been destroyed since.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* ボディで最後に触れていたサブシェイプが離れたときに Exit イベントを
		* 通知する。ボディがその間に破棄されていても通知する。
		*/
		void OnContactRemoved(const JPH::CharacterVirtual* character, const JPH::BodyID& bodyID2, const JPH::SubShapeID& subShapeID2)override;

	private:
		/**
		* [EN]
		* Contact state of one contacted body. Jolt reports contacts per
		* sub-shape, so one body can hold several at once.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 接触先ボディ1つの接触状態。Jolt は接触をサブシェイプごとに報告する
		* ので、1つのボディが同時に複数の接触を持つことがある。
		*/
		struct ContactBody
		{
			/// [EN] Entity represented by the contacted body.
			/// [JP] 接触先ボディが表すエンティティ。
			EntityID entityID_;

			/// [EN] Whether the contacted body is another character's inner body.
			/// [JP] 接触先ボディが他のキャラクターの内側ボディか。
			Bool isCharacter_ = false;

			/// [EN] Number of the body's sub-shapes currently touched.
			/// [JP] 現在触れている、そのボディのサブシェイプ数。
			Uint32 contactCount_ = 0;
		};

		/// [EN] World that owns the character entity receiving contact events.
		/// [JP] 接触イベントを受け取るキャラクターエンティティを所有する World。
		World* world_ = nullptr;

		/// [EN] Entity represented by the virtual character.
		/// [JP] 仮想キャラクターが表すエンティティ。
		EntityID entityID_;

		/// [EN] Bodies in contact with the character. The entity is kept here because the removal callback only gives the body ID, and the body may already be destroyed.
		/// [JP] キャラクターと接触中のボディ。削除コールバックはボディ ID しか渡さず、ボディが破棄済みのこともあるので、エンティティはここで保持する。
		std::unordered_map<JPH::BodyID, ContactBody> contactBodies_;

		/// [EN] Bodies that already dispatched an enter or stay event during the current character update.
		/// [JP] 現在のキャラクターの更新で既に Enter か Stay のイベントを通知したボディ。
		std::unordered_set<JPH::BodyID> notifiedBodies_;
	};
}
