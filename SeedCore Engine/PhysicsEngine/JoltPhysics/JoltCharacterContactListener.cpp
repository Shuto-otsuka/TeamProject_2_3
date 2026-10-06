#include <PhysicsEngine/JoltPhysics/JoltCharacterContactListener.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <FoundationEngine/World/World.h>

namespace SeedCore
{
	/**
	* [EN]
	* Sets the World and entity that receive this listener's contact events.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* このリスナーの接触イベントを受け取る World とエンティティを設定する。
	*/
	void JoltCharacterContactListener::SetTarget(World* world, EntityID entityID)
	{
		world_ = world;
		entityID_ = entityID;
	}

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
	void JoltCharacterContactListener::Begin()
	{
		notifiedBodies_.clear();
	}

	/**
	* [EN]
	* Dispatches an enter event when the first sub-shape of a body is
	* touched, and a stay event when another one of a body already in
	* contact is touched.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディの最初のサブシェイプに触れたときに Enter イベントを、既に接触中の
	* ボディの別のサブシェイプに触れたときに Stay イベントを通知する。
	*/
	void JoltCharacterContactListener::OnContactAdded(const JPH::CharacterVirtual* character, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& settings)
	{
		if (!world_ || contact.mBodyB.IsInvalid())
		{
			return;
		}

		/// [EN] Jolt reports one contact per sub-shape of the body, so a body is entered only by its first one.
		/// [JP] Jolt はボディのサブシェイプごとに接触を報告するので、ボディの Enter は最初の1つでだけ出す。
		auto it = contactBodies_.find(contact.mBodyB);
		if (it == contactBodies_.end())
		{
			/// [EN] Resolve the contacted body to an entity; the entity and sensor flag are kept for the removal callback, which only gives the body ID.
			/// [JP] 接触先ボディをエンティティへ解決する。削除コールバックはボディ ID しか渡さないので、エンティティとセンサーの区分はここで保持する。
			EntityID otherEntityID = world_->CreatePhysics()->BodyEntityID(contact.mBodyB);
			if (otherEntityID == EntityID{})
			{
				return;
			}

			contactBodies_[contact.mBodyB] = ContactBody{ otherEntityID, contact.mIsSensorB, 1 };
			notifiedBodies_.insert(contact.mBodyB);

			/// [EN] Notify both entities with the event type selected by the contacted body.
			/// [JP] 接触先ボディの種別に応じたイベントを両方のエンティティへ通知する。
			if (contact.mIsSensorB)
			{
				PhysicsSystem::DispatchTriggerEnter(*world_, entityID_, otherEntityID);
				PhysicsSystem::DispatchTriggerEnter(*world_, otherEntityID, entityID_);
			}
			else
			{
				PhysicsSystem::DispatchCollisionEnter(*world_, entityID_, otherEntityID);
				PhysicsSystem::DispatchCollisionEnter(*world_, otherEntityID, entityID_);
			}

			return;
		}

		/// [EN] Another sub-shape touched on a body already in contact means the body is staying.
		/// [JP] 既に接触中のボディで別のサブシェイプに触れたのは、ボディとしては接触が続いているということ。
		ContactBody& body = it->second;
		++body.contactCount_;
		if (!notifiedBodies_.insert(contact.mBodyB).second)
		{
			return;
		}

		if (body.isSensor_)
		{
			PhysicsSystem::DispatchTriggerStay(*world_, entityID_, body.entityID_);
			PhysicsSystem::DispatchTriggerStay(*world_, body.entityID_, entityID_);
		}
		else
		{
			PhysicsSystem::DispatchCollisionStay(*world_, entityID_, body.entityID_);
			PhysicsSystem::DispatchCollisionStay(*world_, body.entityID_, entityID_);
		}
	}

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
	void JoltCharacterContactListener::OnContactPersisted(const JPH::CharacterVirtual* character, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& settings)
	{
		if (!world_ || contact.mBodyB.IsInvalid())
		{
			return;
		}

		auto it = contactBodies_.find(contact.mBodyB);
		if (it == contactBodies_.end())
		{
			return;
		}

		/// [EN] Every touched sub-shape reports a persisted contact, but the body stays only once per update.
		/// [JP] 触れているサブシェイプごとに継続の報告が来るが、ボディの Stay は更新1回につき1回だけ。
		if (!notifiedBodies_.insert(contact.mBodyB).second)
		{
			return;
		}

		const ContactBody& body = it->second;
		if (body.isSensor_)
		{
			PhysicsSystem::DispatchTriggerStay(*world_, entityID_, body.entityID_);
			PhysicsSystem::DispatchTriggerStay(*world_, body.entityID_, entityID_);
		}
		else
		{
			PhysicsSystem::DispatchCollisionStay(*world_, entityID_, body.entityID_);
			PhysicsSystem::DispatchCollisionStay(*world_, body.entityID_, entityID_);
		}
	}

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
	void JoltCharacterContactListener::OnContactRemoved(const JPH::CharacterVirtual* character, const JPH::BodyID& bodyID2, const JPH::SubShapeID& subShapeID2)
	{
		auto it = contactBodies_.find(bodyID2);
		if (it == contactBodies_.end())
		{
			return;
		}

		/// [EN] The body exits only when its last touched sub-shape is released.
		/// [JP] ボディが Exit するのは、最後に触れていたサブシェイプが離れたときだけ。
		ContactBody& body = it->second;
		--body.contactCount_;
		if (body.contactCount_ > 0)
		{
			return;
		}

		/// [EN] The entity kept at enter is used, so the exit still reaches the character when the body has been destroyed.
		/// [JP] Enter 時に保持したエンティティを使うので、ボディが破棄されていても Exit はキャラクターへ届く。
		EntityID otherEntityID = body.entityID_;
		Bool isSensor = body.isSensor_;
		contactBodies_.erase(it);

		if (!world_)
		{
			return;
		}

		if (isSensor)
		{
			PhysicsSystem::DispatchTriggerExit(*world_, entityID_, otherEntityID);
			PhysicsSystem::DispatchTriggerExit(*world_, otherEntityID, entityID_);
		}
		else
		{
			PhysicsSystem::DispatchCollisionExit(*world_, entityID_, otherEntityID);
			PhysicsSystem::DispatchCollisionExit(*world_, otherEntityID, entityID_);
		}
	}
}
