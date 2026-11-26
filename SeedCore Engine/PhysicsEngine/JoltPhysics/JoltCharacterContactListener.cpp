#include <PhysicsEngine/JoltPhysics/JoltConfig.h>
#include <External/JoltPhysics/Jolt/Jolt.h>
#include <External/JoltPhysics/Jolt/Physics/Body/BodyID.h>
#include <External/JoltPhysics/Jolt/Physics/Body/MotionType.h>
#include <External/JoltPhysics/Jolt/Physics/Character/CharacterVirtual.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/SubShapeID.h>
#include <PhysicsEngine/JoltPhysics/JoltCharacterContactListener.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <PhysicsEngine/CharacterController/CharacterController.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>

namespace SeedCore
{
	/**
	* [JP]
	* JoltCharacterContactListener の中身。キャラクターからの知らせを受け取る。
	*
	* @brief JoltCharacterContactListener の中身
	*
	* [EN]
	* The inside of JoltCharacterContactListener; it receives news from the character.
	*
	* @brief The inside of JoltCharacterContactListener
	*/
	class JoltCharacterContactListener::Impl final : public JPH::CharacterContactListener
	{
	public:
		/**
		* [JP]
		* 伝える先の世界とアクターを覚える。
		*
		* @brief 準備する
		* @param world    伝える世界
		* @param entityID キャラクターのアクター
		*
		* [EN]
		* Remembers the world and actor to report to.
		*
		* @brief Gets ready
		* @param world    The world to report to
		* @param entityID The character's actor
		*/
		Impl(World& world, EntityID entityID) :world_(&world), entityID_(entityID) {}

		/**
		* [JP]
		* 伝えた記録を空にする。
		*
		* @brief 伝えた記録を空にする
		*
		* [EN]
		* Clears the record of what was reported.
		*
		* @brief Clears the report record
		*/
		void Begin();

	public:
		/**
		* [JP]
		* キャラクターが何かにぶつかり始めたときに呼ばれる。
		*
		* @brief ぶつかり始めを受け取る
		* @param character キャラクター
		* @param contact   ぶつかった相手の情報
		* @param settings  ぶつかり方の設定
		* @note  重なりだけのものと、重さで動く体は知らせない
		*
		* [EN]
		* Called when the character starts bumping into something.
		*
		* @brief Receives a hit starting
		* @param character The character
		* @param contact   What it bumped into
		* @param settings  How the contact behaves
		* @note  Overlap-only bodies and bodies moved by weight are not reported
		*/
		void OnContactAdded(const JPH::CharacterVirtual* character, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& settings)override;

		/**
		* [JP]
		* キャラクターが何かにぶつかり続けているときに呼ばれる。
		*
		* @brief ぶつかり中を受け取る
		* @param character キャラクター
		* @param contact   ぶつかっている相手の情報
		* @param settings  ぶつかり方の設定
		*
		* [EN]
		* Called while the character keeps bumping into something.
		*
		* @brief Receives an ongoing hit
		* @param character The character
		* @param contact   What it is bumping into
		* @param settings  How the contact behaves
		*/
		void OnContactPersisted(const JPH::CharacterVirtual* character, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& settings)override;

		/**
		* [JP]
		* キャラクターが何かから離れたときに呼ばれる。
		*
		* @brief 離れたことを受け取る
		* @param character   キャラクター
		* @param bodyID2     離れた相手の体
		* @param subShapeID2 離れた相手の形の部分
		*
		* [EN]
		* Called when the character moves away from something.
		*
		* @brief Receives a separation
		* @param character   The character
		* @param bodyID2     The body it moved away from
		* @param subShapeID2 The part of that body's shape
		*/
		void OnContactRemoved(const JPH::CharacterVirtual* character, const JPH::BodyID& bodyID2, const JPH::SubShapeID& subShapeID2)override;

	private:
		/**
		* [JP]
		* ぶつかっている相手の記録。
		*
		* @brief ぶつかっている相手
		*
		* [EN]
		* A record of something being touched.
		*
		* @brief Something being touched
		*/
		struct ContactBody
		{
			/// [JP] 相手のアクター
			/// [EN] The other actor
			EntityID entityID_;

			/// [JP] 相手もキャラクターか。キャラクター同士は片側だけ知らせる
			/// [EN] Whether it is also a character; between characters only one side is told
			Bool isCharacter_ = false;

			/// [JP] 触れている部分の数
			/// [EN] How many parts are touching
			Uint32 contactCount_ = 0;
		};

		/// [JP] 伝える世界
		/// [EN] The world to report to
		World* world_ = nullptr;

		/// [JP] キャラクターのアクター
		/// [EN] The character's actor
		EntityID entityID_;

		/// [JP] 今ぶつかっている相手
		/// [EN] What is being touched right now
		std::unordered_map<JPH::BodyID, ContactBody> contactBodies_;

		/// [JP] 今回もう伝えた相手
		/// [EN] What was already reported this time
		std::unordered_set<JPH::BodyID> notifiedBodies_;
	};

	/**
	* [JP]
	* 伝えた記録を空にする。
	*
	* @brief 伝えた記録を空にする
	*
	* [EN]
	* Clears the record of what was reported.
	*
	* @brief Clears the report record
	*/
	void JoltCharacterContactListener::Impl::Begin()
	{
		notifiedBodies_.clear();
	}

	/**
	* [JP]
	* キャラクターが何かにぶつかり始めたときに呼ばれる。
	*
	* @brief ぶつかり始めを受け取る
	* @param character キャラクター
	* @param contact   ぶつかった相手の情報
	* @param settings  ぶつかり方の設定
	* @note  重なりだけのものと、重さで動く体は知らせない
	*
	* [EN]
	* Called when the character starts bumping into something.
	*
	* @brief Receives a hit starting
	* @param character The character
	* @param contact   What it bumped into
	* @param settings  How the contact behaves
	* @note  Overlap-only bodies and bodies moved by weight are not reported
	*/
	void JoltCharacterContactListener::Impl::OnContactAdded(const JPH::CharacterVirtual* character, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& settings)
	{
		if (!world_ || contact.mBodyB.IsInvalid())
		{
			return;
		}

		/// [JP] 重なりだけのものと、重さで動く体は知らせない
		/// [EN] Overlap-only bodies and bodies moved by weight are not reported
		if (contact.mIsSensorB || contact.mMotionTypeB == JPH::EMotionType::Dynamic)
		{
			return;
		}

		/// [JP] 初めての相手なら記録して、ぶつかり始めを知らせる
		/// [EN] For something new, record it and report a hit starting
		auto it = contactBodies_.find(contact.mBodyB);
		if (it == contactBodies_.end())
		{
			EntityID otherEntityID = world_->GetService<Physics>()->BodyEntityID(contact.mBodyB.GetIndexAndSequenceNumber());
			if (otherEntityID == EntityID{})
			{
				return;
			}

			Bool isCharacter = WorldAccessor::Make(*world_, otherEntityID).GetComponent<CharacterController>() != nullptr;
			contactBodies_[contact.mBodyB] = ContactBody{ otherEntityID,isCharacter,1 };
			notifiedBodies_.insert(contact.mBodyB);

			PhysicsSystem::DispatchCollisionEnter(*world_, entityID_, otherEntityID);
			if (!isCharacter)
			{
				PhysicsSystem::DispatchCollisionEnter(*world_, otherEntityID, entityID_);
			}

			return;
		}

		/// [JP] 知っている相手なら、続いていることを今回1回だけ知らせる
		/// [EN] For something known, report ongoing just once this time
		ContactBody& body = it->second;
		++body.contactCount_;
		if (!notifiedBodies_.insert(contact.mBodyB).second)
		{
			return;
		}

		PhysicsSystem::DispatchCollisionStay(*world_, entityID_, body.entityID_);
		if (!body.isCharacter_)
		{
			PhysicsSystem::DispatchCollisionStay(*world_, body.entityID_, entityID_);
		}
	}

	/**
	* [JP]
	* キャラクターが何かにぶつかり続けているときに呼ばれる。
	*
	* @brief ぶつかり中を受け取る
	* @param character キャラクター
	* @param contact   ぶつかっている相手の情報
	* @param settings  ぶつかり方の設定
	*
	* [EN]
	* Called while the character keeps bumping into something.
	*
	* @brief Receives an ongoing hit
	* @param character The character
	* @param contact   What it is bumping into
	* @param settings  How the contact behaves
	*/
	void JoltCharacterContactListener::Impl::OnContactPersisted(const JPH::CharacterVirtual* character, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& settings)
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

		if (!notifiedBodies_.insert(contact.mBodyB).second)
		{
			return;
		}

		const ContactBody& body = it->second;
		PhysicsSystem::DispatchCollisionStay(*world_, entityID_, body.entityID_);
		if (!body.isCharacter_)
		{
			PhysicsSystem::DispatchCollisionStay(*world_, body.entityID_, entityID_);
		}
	}

	/**
	* [JP]
	* キャラクターが何かから離れたときに呼ばれる。
	*
	* @brief 離れたことを受け取る
	* @param character   キャラクター
	* @param bodyID2     離れた相手の体
	* @param subShapeID2 離れた相手の形の部分
	*
	* [EN]
	* Called when the character moves away from something.
	*
	* @brief Receives a separation
	* @param character   The character
	* @param bodyID2     The body it moved away from
	* @param subShapeID2 The part of that body's shape
	*/
	void JoltCharacterContactListener::Impl::OnContactRemoved(const JPH::CharacterVirtual* character, const JPH::BodyID& bodyID2, const JPH::SubShapeID& subShapeID2)
	{
		auto it = contactBodies_.find(bodyID2);
		if (it == contactBodies_.end())
		{
			return;
		}

		/// [JP] 触れている部分がまだあれば、離れていない
		/// [EN] If some part still touches, it has not moved away
		ContactBody& body = it->second;
		--body.contactCount_;
		if (body.contactCount_ > 0)
		{
			return;
		}

		/// [JP] 記録を消してから、離れたことを知らせる
		/// [EN] Remove the record, then report that it moved away
		EntityID otherEntityID = body.entityID_;
		Bool isCharacter = body.isCharacter_;
		contactBodies_.erase(it);

		if (!world_)
		{
			return;
		}

		PhysicsSystem::DispatchCollisionExit(*world_, entityID_, otherEntityID);
		if (!isCharacter)
		{
			PhysicsSystem::DispatchCollisionExit(*world_, otherEntityID, entityID_);
		}
	}

	/**
	* [JP]
	* 伝える先の世界とアクターを覚えて準備する。
	*
	* @brief 準備する
	* @param world    伝える世界
	* @param entityID キャラクターのアクター
	*
	* [EN]
	* Remembers the world and actor to report to and gets ready.
	*
	* @brief Gets ready
	* @param world    The world to report to
	* @param entityID The character's actor
	*/
	JoltCharacterContactListener::JoltCharacterContactListener(World& world, EntityID entityID) :impl_(MakePtr<Impl>(world, entityID))
	{
		/// No Code
	}

	JoltCharacterContactListener::~JoltCharacterContactListener() = default;

	/**
	* [JP]
	* キャラクターに、知らせの受け取り先として登録する。
	*
	* @brief 受け取り先として登録する
	* @param character 登録するキャラクター
	*
	* [EN]
	* Registers itself with the character as the place to send news.
	*
	* @brief Registers as the listener
	* @param character The character to register with
	*/
	void JoltCharacterContactListener::Attach(ScCharacterHandle character)
	{
		static_cast<JPH::CharacterVirtual*>(character)->SetListener(impl_.get());
	}

	/**
	* [JP]
	* 伝えた記録を空にする。
	*
	* @brief 伝えた記録を空にする
	*
	* [EN]
	* Clears the record of what was reported.
	*
	* @brief Clears the report record
	*/
	void JoltCharacterContactListener::Begin()
	{
		impl_->Begin();
	}
}
