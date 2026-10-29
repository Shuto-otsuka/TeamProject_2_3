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
	* [EN]
	* The Jolt character contact listener behind JoltCharacterContactListener.
	* Holds the per-body contact state and turns Jolt's per-sub-shape
	* callbacks into one enter, stay and exit per contacted entity.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* JoltCharacterContactListener の奥にある Jolt のキャラクター接触リスナー。
	* ボディごとの接触状態を持ち、Jolt のサブシェイプごとのコールバックを、
	* 接触先エンティティごとの Enter・Stay・Exit 1回ずつにまとめる。
	*/
	class JoltCharacterContactListener::Impl final : public JPH::CharacterContactListener
	{
	public:
		/**
		* [EN]
		* Creates the listener for the character represented by entityID in
		* world.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* world の中で entityID が表すキャラクター用のリスナーを作る。
		*/
		Impl(World& world, EntityID entityID) :world_(&world), entityID_(entityID) {}

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
	void JoltCharacterContactListener::Impl::Begin()
	{
		notifiedBodies_.clear();
	}

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
	void JoltCharacterContactListener::Impl::OnContactAdded(const JPH::CharacterVirtual* character, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& settings)
	{
		/// [EN] A contact with no body (another virtual character's shape) has no entity to notify.
		/// [JP] ボディの無い接触（他の仮想キャラクターの形状）には、通知するエンティティが無い。
		if (!world_ || contact.mBodyB.IsInvalid())
		{
			return;
		}

		/// [EN] Dynamic bodies and sensors always pair with the kinematic inner body, so the world's contact listener reports them; handling them here too would notify twice.
		/// [JP] 動的ボディとセンサーはキネマティックな内側ボディと必ず組になり、ワールドの接触リスナーが知らせる。ここでも扱うと二重に通知される。
		if (contact.mIsSensorB || contact.mMotionTypeB == JPH::EMotionType::Dynamic)
		{
			return;
		}

		/// [EN] Jolt reports one contact per sub-shape of the body, so a body is entered only by its first one.
		/// [JP] Jolt はボディのサブシェイプごとに接触を報告するので、ボディの Enter は最初の1つでだけ出す。
		auto it = contactBodies_.find(contact.mBodyB);
		if (it == contactBodies_.end())
		{
			/// [EN] Resolve the contacted body to an entity; the entity and whether it is a character are kept for the removal callback, which only gives the body ID.
			/// [JP] 接触先ボディをエンティティへ解決する。削除コールバックはボディ ID しか渡さないので、エンティティとキャラクターかどうかはここで保持する。
			EntityID otherEntityID = world_->GetService<Physics>()->BodyEntityID(contact.mBodyB.GetIndexAndSequenceNumber());
			if (otherEntityID == EntityID{})
			{
				return;
			}

			/// [EN] A body owned by a CharacterController entity is another character's inner body.
			/// [JP] CharacterController を持つエンティティのボディは、他のキャラクターの内側ボディ。
			Bool isCharacter = WorldAccessor::Make(*world_, otherEntityID).GetComponent<CharacterController>() != nullptr;
			contactBodies_[contact.mBodyB] = ContactBody{ otherEntityID,isCharacter,1 };
			notifiedBodies_.insert(contact.mBodyB);

			/// [EN] The owning entity is always notified; another character also detects this contact in its own update and notifies itself, so only a non-character is notified from here.
			/// [JP] 所有エンティティには常に通知する。他のキャラクターは自分の更新でこの接触を検出して自分へ通知するので、ここから相手へ通知するのはキャラクターでないときだけ。
			PhysicsSystem::DispatchCollisionEnter(*world_, entityID_, otherEntityID);
			if (!isCharacter)
			{
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

		/// [EN] Notified the same way as the enter event: another character notifies itself.
		/// [JP] Enter と同じく通知する。他のキャラクターへは、そのキャラクター自身が通知する。
		PhysicsSystem::DispatchCollisionStay(*world_, entityID_, body.entityID_);
		if (!body.isCharacter_)
		{
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
	void JoltCharacterContactListener::Impl::OnContactPersisted(const JPH::CharacterVirtual* character, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& settings)
	{
		if (!world_ || contact.mBodyB.IsInvalid())
		{
			return;
		}

		/// [EN] Only bodies that were entered here can stay; dynamic bodies and sensors were never recorded.
		/// [JP] Stay できるのは、ここで Enter したボディだけ。動的ボディとセンサーは記録していない。
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

		/// [EN] Notified the same way as the enter event: another character notifies itself.
		/// [JP] Enter と同じく通知する。他のキャラクターへは、そのキャラクター自身が通知する。
		const ContactBody& body = it->second;
		PhysicsSystem::DispatchCollisionStay(*world_, entityID_, body.entityID_);
		if (!body.isCharacter_)
		{
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
	void JoltCharacterContactListener::Impl::OnContactRemoved(const JPH::CharacterVirtual* character, const JPH::BodyID& bodyID2, const JPH::SubShapeID& subShapeID2)
	{
		/// [EN] Only bodies that were entered here can exit; dynamic bodies and sensors were never recorded.
		/// [JP] Exit できるのは、ここで Enter したボディだけ。動的ボディとセンサーは記録していない。
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
		Bool isCharacter = body.isCharacter_;
		contactBodies_.erase(it);

		if (!world_)
		{
			return;
		}

		/// [EN] Notified the same way as the enter event: another character notifies itself.
		/// [JP] Enter と同じく通知する。他のキャラクターへは、そのキャラクター自身が通知する。
		PhysicsSystem::DispatchCollisionExit(*world_, entityID_, otherEntityID);
		if (!isCharacter)
		{
			PhysicsSystem::DispatchCollisionExit(*world_, otherEntityID, entityID_);
		}
	}

	/**
	* [EN]
	* Creates a listener that dispatches contacts of the character
	* represented by entityID into world.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* entityID が表すキャラクターの接触を world へ通知するリスナーを作る。
	*/
	JoltCharacterContactListener::JoltCharacterContactListener(World& world, EntityID entityID) :impl_(MakePtr<Impl>(world, entityID))
	{
		/// No Code
	}

	/**
	* [EN]
	* Defined here, where Impl is a complete type.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Impl が完全型になるここで定義する。
	*/
	JoltCharacterContactListener::~JoltCharacterContactListener() = default;

	/**
	* [EN]
	* Registers this listener on the given virtual character, so its
	* contacts start arriving here.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 指定の仮想キャラクターにこのリスナーを登録し、その接触がここへ
	* 届くようにする。
	*/
	void JoltCharacterContactListener::Attach(ScCharacterHandle character)
	{
		/// [EN] The handle is the Jolt character itself, kept opaque outside the physics module.
		/// [JP] ハンドルは Jolt のキャラクターそのもので、物理モジュールの外には中身を見せない。
		static_cast<JPH::CharacterVirtual*>(character)->SetListener(impl_.get());
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
		impl_->Begin();
	}
}
