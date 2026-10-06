#include <PhysicsEngine/JoltPhysics/JoltContactListener.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>

namespace SeedCore
{
	/**
	* [EN]
	* Sets the World that receives dispatched contact events.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 接触イベントの通知先となる World を設定する。
	*/
	void JoltContactListener::ActiveWorld(World* world)
	{
		world_ = world;
	}

	/**
	* [EN]
	* Returns the World that receives dispatched contact events.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 接触イベントの通知先となる World を返す。
	*/
	World* JoltContactListener::ActiveWorld()const
	{
		return world_;
	}

	/**
	* [EN]
	* Decides which body pairs that lost their last contact really
	* exited, then drains queued contacts and dispatches their
	* collision or trigger events.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 最後の接触を失ったボディペアのうち本当に離れたものを判定し、その後
	* キュー内の接触を取り出して衝突またはトリガーイベントを通知する。
	*/
	void JoltContactListener::DispatchEvent(const JPH::BodyInterface& bodyInterface)
	{
		DynamicArray<ContactEvent> events;
		{
			std::lock_guard<std::mutex> lock(mutex_);

			/// [EN] A sleeping body loses all its contacts, so a resting pair exits only once neither body is asleep.
			/// [JP] 眠ったボディは接触をすべて失うので、休止中のペアが Exit するのは、どちらのボディも眠っていないときだけ。
			for (auto restingIt = restingPairs_.begin(); restingIt != restingPairs_.end();)
			{
				Uint64 pairKey = *restingIt;
				auto pairIt = contactPairs_.find(pairKey);
				if (pairIt == contactPairs_.end())
				{
					restingIt = restingPairs_.erase(restingIt);
					continue;
				}

				/// [EN] The key holds the smaller body ID in its upper half and the larger one in its lower half.
				/// [JP] キーの上位側に小さいほうのボディ ID、下位側に大きいほうのボディ ID が入っている。
				JPH::BodyID body1ID(static_cast<Uint32>(pairKey >> 32));
				JPH::BodyID body2ID(static_cast<Uint32>(pairKey));

				/// [EN] A destroyed or removed body ends the contact for good.
				/// [JP] 破棄された、またはシミュレーションから外されたボディとの接触は、そこで終わる。
				Bool bothAdded = bodyInterface.IsAdded(body1ID) && bodyInterface.IsAdded(body2ID);

				/// [EN] A static body is never active, so only a non-static body that is not active is asleep.
				/// [JP] スタティックのボディは常に非アクティブなので、眠っているのはスタティック以外で非アクティブなボディだけ。
				Bool asleep = bothAdded && ((bodyInterface.GetMotionType(body1ID) != JPH::EMotionType::Static && !bodyInterface.IsActive(body1ID)) || (bodyInterface.GetMotionType(body2ID) != JPH::EMotionType::Static && !bodyInterface.IsActive(body2ID)));

				if (asleep)
				{
					++restingIt;
					continue;
				}

				/// [EN] Both bodies are awake and the contact did not come back, so the pair really separated.
				/// [JP] 両方のボディが起きていて接触が戻らなかったので、ペアは本当に離れた。
				const ContactPair& pair = pairIt->second;
				pendingEvents_.push_back(ContactEvent{ pair.entityIDs_.first, pair.entityIDs_.second, ContactEventKind::Exit, pair.isSensor_ });
				contactPairs_.erase(pairIt);
				restingIt = restingPairs_.erase(restingIt);
			}

			/// [EN] Move pending events to local storage while holding the callback mutex.
			/// [JP] コールバック用ミューテックスの保持中に、保留イベントをローカルへ移す。
			events.assign(pendingEvents_.begin(), pendingEvents_.end());
			pendingEvents_.clear();

			/// [EN] Each step dispatches at most one enter or stay per body pair, so the record starts over here.
			/// [JP] 1ステップにつきボディペアごとの Enter か Stay は最大1回なので、記録はここでやり直す。
			notifiedPairs_.clear();
		}

		if (!world_)
		{
			return;
		}

		/// [EN] Dispatch each phase symmetrically to both participating entities.
		/// [JP] 各段階のイベントを、参加する両方のエンティティへ対称に通知する。
		for (const ContactEvent& event : events)
		{
			switch (event.kind_)
			{
			case ContactEventKind::Enter:
				if (event.isSensor_)
				{
					PhysicsSystem::DispatchTriggerEnter(*world_, event.entityID_, event.otherEntityID_);
					PhysicsSystem::DispatchTriggerEnter(*world_, event.otherEntityID_, event.entityID_);
				}
				else
				{
					PhysicsSystem::DispatchCollisionEnter(*world_, event.entityID_, event.otherEntityID_);
					PhysicsSystem::DispatchCollisionEnter(*world_, event.otherEntityID_, event.entityID_);
				}
				break;
			case ContactEventKind::Stay:
				if (event.isSensor_)
				{
					PhysicsSystem::DispatchTriggerStay(*world_, event.entityID_, event.otherEntityID_);
					PhysicsSystem::DispatchTriggerStay(*world_, event.otherEntityID_, event.entityID_);
				}
				else
				{
					PhysicsSystem::DispatchCollisionStay(*world_, event.entityID_, event.otherEntityID_);
					PhysicsSystem::DispatchCollisionStay(*world_, event.otherEntityID_, event.entityID_);
				}
				break;
			case ContactEventKind::Exit:
				if (event.isSensor_)
				{
					PhysicsSystem::DispatchTriggerExit(*world_, event.entityID_, event.otherEntityID_);
					PhysicsSystem::DispatchTriggerExit(*world_, event.otherEntityID_, event.entityID_);
				}
				else
				{
					PhysicsSystem::DispatchCollisionExit(*world_, event.entityID_, event.otherEntityID_);
					PhysicsSystem::DispatchCollisionExit(*world_, event.otherEntityID_, event.entityID_);
				}
				break;
			}
		}
	}

	/**
	* [EN]
	* Queues an enter event when the first sub-shape contact of a body
	* pair is added, and a stay event when another one is added to a
	* pair already in contact or to a resting pair picking its contact
	* back up.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディペアで最初のサブシェイプの接触が追加されたときに Enter イベントを、
	* 既に接触中のペアへさらに追加されたとき、または休止中のペアが接触を
	* 取り戻したときに Stay イベントをキューへ積む。
	*/
	void JoltContactListener::OnContactAdded(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings)
	{
		/// [EN] Both body IDs packed into one key; Jolt always passes the smaller ID as body1, so a pair keeps the same key.
		/// [JP] 2つのボディ ID を1つのキーへまとめる。Jolt は常に小さいほうの ID を body1 として渡すので、同じペアは同じキーになる。
		Uint64 pairKey = (static_cast<Uint64>(body1.GetID().GetIndexAndSequenceNumber()) << 32) | body2.GetID().GetIndexAndSequenceNumber();

		std::lock_guard<std::mutex> lock(mutex_);

		ContactPair& pair = contactPairs_[pairKey];

		/// [EN] A resting pair whose body woke up still in contact picks the contact back up instead of entering anew.
		/// [JP] 接触したまま起きた休止中のペアは、新たに Enter せず接触を取り戻す。
		Bool resumed = pair.contactCount_ == 0 && restingPairs_.erase(pairKey) > 0;

		/// [EN] The first sub-shape contact enters the pair; the entities and sensor flag are kept for the removal callback, which only gives body IDs.
		/// [JP] 最初のサブシェイプの接触でペアが Enter する。削除コールバックはボディ ID しか渡さないので、エンティティとセンサーの区分はここで保持する。
		if (pair.contactCount_ == 0 && !resumed)
		{
			pair.entityIDs_ = { std::bit_cast<EntityID>(static_cast<Uint64>(body1.GetUserData())), std::bit_cast<EntityID>(static_cast<Uint64>(body2.GetUserData())) };
			pair.isSensor_ = body1.IsSensor() || body2.IsSensor();

			notifiedPairs_.insert(pairKey);
			pendingEvents_.push_back(ContactEvent{ pair.entityIDs_.first, pair.entityIDs_.second, ContactEventKind::Enter, pair.isSensor_ });
		}
		/// [EN] Another sub-shape touching a pair already in contact, or a resting pair picking its contact back up, means the pair is staying.
		/// [JP] 既に接触中のペアで別のサブシェイプが触れたのも、休止中のペアが接触を取り戻したのも、ペアとしては接触が続いているということ。
		else if (notifiedPairs_.insert(pairKey).second)
		{
			pendingEvents_.push_back(ContactEvent{ pair.entityIDs_.first, pair.entityIDs_.second, ContactEventKind::Stay, pair.isSensor_ });
		}

		++pair.contactCount_;
	}

	/**
	* [EN]
	* Queues a stay event for a body pair still in contact, at most once
	* per step.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 接触が続いているボディペアの Stay イベントを、1ステップにつき最大1回
	* キューへ積む。
	*/
	void JoltContactListener::OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings)
	{
		/// [EN] Both body IDs packed into one key; Jolt always passes the smaller ID as body1, so a pair keeps the same key.
		/// [JP] 2つのボディ ID を1つのキーへまとめる。Jolt は常に小さいほうの ID を body1 として渡すので、同じペアは同じキーになる。
		Uint64 pairKey = (static_cast<Uint64>(body1.GetID().GetIndexAndSequenceNumber()) << 32) | body2.GetID().GetIndexAndSequenceNumber();

		std::lock_guard<std::mutex> lock(mutex_);

		auto it = contactPairs_.find(pairKey);
		if (it == contactPairs_.end())
		{
			return;
		}

		/// [EN] Every touching sub-shape reports a persisted contact, but the pair stays only once per step.
		/// [JP] 触れているサブシェイプごとに継続の報告が来るが、ペアの Stay は1ステップに1回だけ。
		if (notifiedPairs_.insert(pairKey).second)
		{
			const ContactPair& pair = it->second;
			pendingEvents_.push_back(ContactEvent{ pair.entityIDs_.first, pair.entityIDs_.second, ContactEventKind::Stay, pair.isSensor_ });
		}
	}

	/**
	* [EN]
	* Marks a body pair as resting when its last sub-shape contact is
	* removed. Whether it exited or only fell asleep is decided in
	* DispatchEvent.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* ボディペアの最後のサブシェイプの接触が削除されたとき、そのペアを休止中
	* にする。離れたのか眠っただけなのかは DispatchEvent で判定する。
	*/
	void JoltContactListener::OnContactRemoved(const JPH::SubShapeIDPair& subShapePair)
	{
		/// [EN] Both body IDs packed into one key, in the same order as the added and persisted callbacks.
		/// [JP] 2つのボディ ID を1つのキーへまとめる。順序は追加・継続のコールバックと同じ。
		Uint64 pairKey = (static_cast<Uint64>(subShapePair.GetBody1ID().GetIndexAndSequenceNumber()) << 32) | subShapePair.GetBody2ID().GetIndexAndSequenceNumber();

		std::lock_guard<std::mutex> lock(mutex_);

		auto it = contactPairs_.find(pairKey);
		if (it == contactPairs_.end())
		{
			return;
		}

		/// [EN] The pair stops touching only when its last touching sub-shape lets go.
		/// [JP] ペアの接触が途切れるのは、最後に触れていたサブシェイプが離れたときだけ。
		ContactPair& pair = it->second;
		--pair.contactCount_;
		if (pair.contactCount_ > 0)
		{
			return;
		}

		/// [EN] Bodies cannot be inspected during this callback, so whether the pair separated or a body only fell asleep is decided in DispatchEvent.
		/// [JP] このコールバック中はボディを調べられないので、ペアが離れたのかボディが眠っただけなのかは DispatchEvent で判定する。
		restingPairs_.insert(pairKey);
	}
}
