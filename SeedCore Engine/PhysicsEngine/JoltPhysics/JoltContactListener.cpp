#include <PhysicsEngine/JoltPhysics/JoltConfig.h>
#include <External/JoltPhysics/Jolt/Jolt.h>
#include <External/JoltPhysics/Jolt/Physics/PhysicsSystem.h>
#include <External/JoltPhysics/Jolt/Physics/Body/Body.h>
#include <External/JoltPhysics/Jolt/Physics/Body/BodyInterface.h>
#include <External/JoltPhysics/Jolt/Physics/Body/MotionType.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/ContactListener.h>
#include <External/JoltPhysics/Jolt/Physics/Collision/Shape/SubShapeIDPair.h>
#include <PhysicsEngine/JoltPhysics/JoltContactListener.h>
#include <PhysicsEngine/Physics/PhysicsSystem.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	/**
	* [JP]
	* JoltContactListener の中身。物理エンジンから知らせを受け取って貯めておく。
	*
	* @brief JoltContactListener の中身
	*
	* [EN]
	* The inside of JoltContactListener; it receives news from the physics engine and stores it.
	*
	* @brief The inside of JoltContactListener
	*/
	class JoltContactListener::Impl final :public JPH::ContactListener
	{
	private:
		/**
		* [JP]
		* 知らせの種類。
		*
		* @brief 知らせの種類
		*
		* [EN]
		* The kind of news.
		*
		* @brief The kind of news
		*/
		enum class ContactEventKind
		{
			/// [JP] ぶつかり始めた
			/// [EN] Started hitting
			Enter,

			/// [JP] ぶつかり続けている
			/// [EN] Still hitting
			Stay,

			/// [JP] 離れた
			/// [EN] Separated
			Exit,
		};

		/**
		* [JP]
		* 伝える知らせ1つ分。
		*
		* @brief 知らせ1つ
		*
		* [EN]
		* One piece of news to pass on.
		*
		* @brief One piece of news
		*/
		struct ContactEvent
		{
			/// [JP] 1つめのアクター
			/// [EN] The first actor
			EntityID entityID_;

			/// [JP] 2つめのアクター
			/// [EN] The second actor
			EntityID otherEntityID_;

			/// [JP] 知らせの種類
			/// [EN] The kind of news
			ContactEventKind kind_ = ContactEventKind::Enter;

			/// [JP] 重なりだけを知らせるものか
			/// [EN] Whether it is an overlap-only contact
			Bool isSensor_ = false;
		};

		/**
		* [JP]
		* ぶつかっている2つの体の組の記録。
		*
		* @brief ぶつかっている組
		*
		* [EN]
		* A record of two bodies that are touching.
		*
		* @brief A touching pair
		*/
		struct ContactPair
		{
			/// [JP] 2つのアクター
			/// [EN] The two actors
			std::pair<EntityID, EntityID> entityIDs_;

			/// [JP] 重なりだけを知らせるものか
			/// [EN] Whether it is an overlap-only contact
			Bool isSensor_ = false;

			/// [JP] 触れている部分の数。0 になったら離れた
			/// [EN] How many parts are touching; 0 means they separated
			Uint32 contactCount_ = 0;
		};

	public:
		/**
		* [JP]
		* 貯めた知らせを、アクターのスクリプトにまとめて伝える。
		*
		* @brief 知らせを伝える
		* @param bodyInterface 体の状態を調べる窓口
		* @param world         伝える世界
		* @note  眠っている体の組は、離れたことにしない
		*
		* [EN]
		* Passes the stored news to the actors' scripts all together.
		*
		* @brief Reports the news
		* @param bodyInterface The door used to check bodies
		* @param world         The world to report to
		* @note  Pairs whose bodies are asleep are not treated as separated
		*/
		void DispatchEvent(const JPH::BodyInterface& bodyInterface, World& world);

		/**
		* [JP]
		* 物理エンジンが「ぶつかり始めた」と知らせてきたときに呼ばれる。
		*
		* @brief ぶつかり始めを受け取る
		* @param body1    1つめの体
		* @param body2    2つめの体
		* @param manifold ぶつかった場所の情報
		* @param settings ぶつかり方の設定
		*
		* [EN]
		* Called when the physics engine says two bodies started touching.
		*
		* @brief Receives a hit starting
		* @param body1    The first body
		* @param body2    The second body
		* @param manifold Where they touched
		* @param settings How the contact behaves
		*/
		void OnContactAdded(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings)override;

		/**
		* [JP]
		* 物理エンジンが「ぶつかり続けている」と知らせてきたときに呼ばれる。
		*
		* @brief ぶつかり中を受け取る
		* @param body1    1つめの体
		* @param body2    2つめの体
		* @param manifold ぶつかった場所の情報
		* @param settings ぶつかり方の設定
		*
		* [EN]
		* Called when the physics engine says two bodies are still touching.
		*
		* @brief Receives an ongoing hit
		* @param body1    The first body
		* @param body2    The second body
		* @param manifold Where they touch
		* @param settings How the contact behaves
		*/
		void OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings)override;

		/**
		* [JP]
		* 物理エンジンが「離れた」と知らせてきたときに呼ばれる。
		*
		* @brief 離れたことを受け取る
		* @param subShapePair 離れた2つの体の組
		*
		* [EN]
		* Called when the physics engine says two bodies separated.
		*
		* @brief Receives a separation
		* @param subShapePair The pair of bodies that separated
		*/
		void OnContactRemoved(const JPH::SubShapeIDPair& subShapePair)override;

	private:
		/// [JP] 物理エンジンは別のスレッドから知らせてくるので、守りながら触る
		/// [EN] The physics engine calls from other threads, so guard access
		std::mutex mutex_;

		/// [JP] まだ伝えていない知らせ
		/// [EN] News not passed on yet
		DynamicArray<ContactEvent> pendingEvents_;

		/// [JP] 今ぶつかっている組
		/// [EN] The pairs touching right now
		std::unordered_map<Uint64, ContactPair> contactPairs_;

		/// [JP] このフレームでもう伝えた組。同じ知らせを2回出さない
		/// [EN] Pairs already reported this frame, so nothing is sent twice
		std::unordered_set<Uint64> notifiedPairs_;

		/// [JP] 触れている部分がなくなった組。眠っているだけかを次で確かめる
		/// [EN] Pairs with nothing touching; next time we check whether they are just asleep
		std::unordered_set<Uint64> restingPairs_;
	};

	/**
	* [JP]
	* 貯めた知らせを、アクターのスクリプトにまとめて伝える。
	*
	* @brief 知らせを伝える
	* @param bodyInterface 体の状態を調べる窓口
	* @param world         伝える世界
	* @note  眠っている体の組は、離れたことにしない
	*
	* [EN]
	* Passes the stored news to the actors' scripts all together.
	*
	* @brief Reports the news
	* @param bodyInterface The door used to check bodies
	* @param world         The world to report to
	* @note  Pairs whose bodies are asleep are not treated as separated
	*/
	void JoltContactListener::Impl::DispatchEvent(const JPH::BodyInterface& bodyInterface, World& world)
	{
		DynamicArray<ContactEvent> events;
		{
			std::lock_guard<std::mutex> lock(mutex_);

			/// [JP] 触れなくなった組が、本当に離れたのか眠っているだけかを確かめる
			/// [EN] Check whether a pair that stopped touching really separated or is just asleep
			for (auto restingIt = restingPairs_.begin(); restingIt != restingPairs_.end();)
			{
				Uint64 pairKey = *restingIt;
				auto pairIt = contactPairs_.find(pairKey);
				if (pairIt == contactPairs_.end())
				{
					restingIt = restingPairs_.erase(restingIt);
					continue;
				}

				JPH::BodyID body1ID(static_cast<Uint32>(pairKey >> 32));
				JPH::BodyID body2ID(static_cast<Uint32>(pairKey));

				Bool bothAdded = bodyInterface.IsAdded(body1ID) && bodyInterface.IsAdded(body2ID);

				Bool asleep = bothAdded && ((bodyInterface.GetMotionType(body1ID) != JPH::EMotionType::Static && !bodyInterface.IsActive(body1ID)) || (bodyInterface.GetMotionType(body2ID) != JPH::EMotionType::Static && !bodyInterface.IsActive(body2ID)));

				if (asleep)
				{
					++restingIt;
					continue;
				}

				const ContactPair& pair = pairIt->second;
				pendingEvents_.push_back(ContactEvent{ pair.entityIDs_.first, pair.entityIDs_.second, ContactEventKind::Exit, pair.isSensor_ });
				contactPairs_.erase(pairIt);
				restingIt = restingPairs_.erase(restingIt);
			}

			/// [JP] 貯めた知らせを取り出して空にする
			/// [EN] Take out the stored news and empty the store
			events.assign(pendingEvents_.begin(), pendingEvents_.end());
			pendingEvents_.clear();

			notifiedPairs_.clear();
		}

		/// [JP] 種類ごとに、両方のアクターへ知らせる
		/// [EN] Tell both actors, by kind of news
		for (const ContactEvent& event : events)
		{
			switch (event.kind_)
			{
			case ContactEventKind::Enter:
				if (event.isSensor_)
				{
					PhysicsSystem::DispatchTriggerEnter(world, event.entityID_, event.otherEntityID_);
					PhysicsSystem::DispatchTriggerEnter(world, event.otherEntityID_, event.entityID_);
				}
				else
				{
					PhysicsSystem::DispatchCollisionEnter(world, event.entityID_, event.otherEntityID_);
					PhysicsSystem::DispatchCollisionEnter(world, event.otherEntityID_, event.entityID_);
				}
				break;
			case ContactEventKind::Stay:
				if (event.isSensor_)
				{
					PhysicsSystem::DispatchTriggerStay(world, event.entityID_, event.otherEntityID_);
					PhysicsSystem::DispatchTriggerStay(world, event.otherEntityID_, event.entityID_);
				}
				else
				{
					PhysicsSystem::DispatchCollisionStay(world, event.entityID_, event.otherEntityID_);
					PhysicsSystem::DispatchCollisionStay(world, event.otherEntityID_, event.entityID_);
				}
				break;
			case ContactEventKind::Exit:
				if (event.isSensor_)
				{
					PhysicsSystem::DispatchTriggerExit(world, event.entityID_, event.otherEntityID_);
					PhysicsSystem::DispatchTriggerExit(world, event.otherEntityID_, event.entityID_);
				}
				else
				{
					PhysicsSystem::DispatchCollisionExit(world, event.entityID_, event.otherEntityID_);
					PhysicsSystem::DispatchCollisionExit(world, event.otherEntityID_, event.entityID_);
				}
				break;
			}
		}
	}

	/**
	* [JP]
	* 物理エンジンが「ぶつかり始めた」と知らせてきたときに呼ばれる。
	*
	* @brief ぶつかり始めを受け取る
	* @param body1    1つめの体
	* @param body2    2つめの体
	* @param manifold ぶつかった場所の情報
	* @param settings ぶつかり方の設定
	*
	* [EN]
	* Called when the physics engine says two bodies started touching.
	*
	* @brief Receives a hit starting
	* @param body1    The first body
	* @param body2    The second body
	* @param manifold Where they touched
	* @param settings How the contact behaves
	*/
	void JoltContactListener::Impl::OnContactAdded(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings)
	{
		/// [JP] 2つの体の番号から、組を表す鍵を作る
		/// [EN] Make a key for the pair from the two body numbers
		Uint64 pairKey = (static_cast<Uint64>(body1.GetID().GetIndexAndSequenceNumber()) << 32) | body2.GetID().GetIndexAndSequenceNumber();

		std::lock_guard<std::mutex> lock(mutex_);

		ContactPair& pair = contactPairs_[pairKey];

		/// [JP] 眠っていただけの組が戻ってきたなら、始まりとしては扱わない
		/// [EN] A pair that was only asleep coming back is not a new start
		Bool resumed = pair.contactCount_ == 0 && restingPairs_.erase(pairKey) > 0;

		/// [JP] 初めてなら始まりを、そうでなければ続いていることを1回だけ知らせる
		/// [EN] Report a start the first time, otherwise report ongoing just once
		if (pair.contactCount_ == 0 && !resumed)
		{
			pair.entityIDs_ = { std::bit_cast<EntityID>(static_cast<Uint64>(body1.GetUserData())), std::bit_cast<EntityID>(static_cast<Uint64>(body2.GetUserData())) };
			pair.isSensor_ = body1.IsSensor() || body2.IsSensor();

			notifiedPairs_.insert(pairKey);
			pendingEvents_.push_back(ContactEvent{ pair.entityIDs_.first, pair.entityIDs_.second, ContactEventKind::Enter, pair.isSensor_ });
		}
		else if (notifiedPairs_.insert(pairKey).second)
		{
			pendingEvents_.push_back(ContactEvent{ pair.entityIDs_.first, pair.entityIDs_.second, ContactEventKind::Stay, pair.isSensor_ });
		}

		++pair.contactCount_;
	}

	/**
	* [JP]
	* 物理エンジンが「ぶつかり続けている」と知らせてきたときに呼ばれる。
	*
	* @brief ぶつかり中を受け取る
	* @param body1    1つめの体
	* @param body2    2つめの体
	* @param manifold ぶつかった場所の情報
	* @param settings ぶつかり方の設定
	*
	* [EN]
	* Called when the physics engine says two bodies are still touching.
	*
	* @brief Receives an ongoing hit
	* @param body1    The first body
	* @param body2    The second body
	* @param manifold Where they touch
	* @param settings How the contact behaves
	*/
	void JoltContactListener::Impl::OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings)
	{
		Uint64 pairKey = (static_cast<Uint64>(body1.GetID().GetIndexAndSequenceNumber()) << 32) | body2.GetID().GetIndexAndSequenceNumber();

		std::lock_guard<std::mutex> lock(mutex_);

		auto it = contactPairs_.find(pairKey);
		if (it == contactPairs_.end())
		{
			return;
		}

		/// [JP] このフレームでまだ伝えていなければ、続いていることを知らせる
		/// [EN] Report ongoing if it has not been reported this frame
		if (notifiedPairs_.insert(pairKey).second)
		{
			const ContactPair& pair = it->second;
			pendingEvents_.push_back(ContactEvent{ pair.entityIDs_.first, pair.entityIDs_.second, ContactEventKind::Stay, pair.isSensor_ });
		}
	}

	/**
	* [JP]
	* 物理エンジンが「離れた」と知らせてきたときに呼ばれる。
	*
	* @brief 離れたことを受け取る
	* @param subShapePair 離れた2つの体の組
	*
	* [EN]
	* Called when the physics engine says two bodies separated.
	*
	* @brief Receives a separation
	* @param subShapePair The pair of bodies that separated
	*/
	void JoltContactListener::Impl::OnContactRemoved(const JPH::SubShapeIDPair& subShapePair)
	{
		Uint64 pairKey = (static_cast<Uint64>(subShapePair.GetBody1ID().GetIndexAndSequenceNumber()) << 32) | subShapePair.GetBody2ID().GetIndexAndSequenceNumber();

		std::lock_guard<std::mutex> lock(mutex_);

		auto it = contactPairs_.find(pairKey);
		if (it == contactPairs_.end())
		{
			return;
		}

		/// [JP] 触れている部分がまだあれば、離れていない
		/// [EN] If some part still touches, they have not separated
		ContactPair& pair = it->second;
		--pair.contactCount_;
		if (pair.contactCount_ > 0)
		{
			return;
		}

		/// [JP] 離れたかどうかは、次にまとめて伝えるときに確かめる
		/// [EN] Whether they really separated is checked at the next report
		restingPairs_.insert(pairKey);
	}

	/**
	* [JP]
	* 中身を作る。
	*
	* @brief 中身を作る
	*
	* [EN]
	* Makes the inner parts.
	*
	* @brief Makes the inner parts
	*/
	JoltContactListener::JoltContactListener() :impl_(MakePtr<Impl>())
	{
		/// No Code
	}

	JoltContactListener::~JoltContactListener() = default;

	/**
	* [JP]
	* 物理エンジンに、知らせの受け取り先として登録する。
	*
	* @brief 受け取り先として登録する
	* @param physicsSystem 登録する物理エンジン
	*
	* [EN]
	* Registers itself with the physics engine as the place to send news.
	*
	* @brief Registers as the listener
	* @param physicsSystem The physics engine to register with
	*/
	void JoltContactListener::Attach(JPH::PhysicsSystem& physicsSystem)
	{
		physicsSystem.SetContactListener(impl_.get());
	}

	/**
	* [JP]
	* 集めた知らせを、アクターのスクリプトにまとめて伝える。
	*
	* @brief 知らせを伝える
	* @param physicsSystem 体の状態を調べる物理エンジン
	* @param world         伝える世界
	*
	* [EN]
	* Passes the collected news to the actors' scripts all together.
	*
	* @brief Reports the news
	* @param physicsSystem The physics engine used to check bodies
	* @param world         The world to report to
	*/
	void JoltContactListener::DispatchEvent(JPH::PhysicsSystem& physicsSystem, World& world)
	{
		impl_->DispatchEvent(physicsSystem.GetBodyInterface(), world);
	}
}
