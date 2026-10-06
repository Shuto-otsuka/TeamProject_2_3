#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	class World;

	/**
	* [EN]
	* Collects Jolt body-contact callbacks and dispatches SeedCore collision
	* and trigger events on the active World. Jolt reports contacts per
	* sub-shape pair; they are merged per body pair, so each pair of
	* entities gets one enter, at most one stay per step, and one exit.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* Jolt のボディ接触コールバックを収集し、アクティブな World へ
	* SeedCore の衝突・トリガーイベントを通知する。Jolt は接触を
	* サブシェイプのペアごとに報告するが、ここではボディペアごとにまとめ、
	* エンティティの組ごとに Enter を1回、Stay を1ステップ最大1回、Exit を1回通知する。
	*/
	class JoltContactListener final :public JPH::ContactListener
	{
	public:
		/**
		* [EN]
		* Identifies the lifecycle phase of a queued contact event.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* キューへ積む接触イベントのライフサイクル段階を表す。
		*/
		enum class ContactEventKind
		{
			/// [EN] Contact began during the physics update.
			/// [JP] 物理更新中に接触が始まった。
			Enter,

			/// [EN] Contact continued during the physics update.
			/// [JP] 物理更新中に接触が継続した。
			Stay,

			/// [EN] Contact ended during the physics update.
			/// [JP] 物理更新中に接触が終了した。
			Exit,
		};

		/**
		* [EN]
		* Deferred contact notification transferred from physics callbacks.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 物理コールバックから引き渡される遅延接触通知。
		*/
		struct ContactEvent
		{
			/// [EN] Entity represented by the first body.
			/// [JP] 1つ目のボディが表すエンティティ。
			EntityID entityID_;

			/// [EN] Entity represented by the second body.
			/// [JP] 2つ目のボディが表すエンティティ。
			EntityID otherEntityID_;

			/// [EN] Lifecycle phase dispatched for this contact.
			/// [JP] この接触で通知するライフサイクル段階。
			ContactEventKind kind_ = ContactEventKind::Enter;

			/// [EN] Whether either contacted body is a sensor.
			/// [JP] 接触したボディのいずれかがセンサーか。
			Bool isSensor_ = false;
		};

	public:
		/**
		* [EN]
		* Sets the World that receives dispatched contact events.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 接触イベントの通知先となる World を設定する。
		*/
		void ActiveWorld(World* world);

		/**
		* [EN]
		* Returns the World that receives dispatched contact events.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 接触イベントの通知先となる World を返す。
		*/
		World* ActiveWorld()const;

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
		void DispatchEvent(const JPH::BodyInterface& bodyInterface);

	public:
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
		void OnContactAdded(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings)override;

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
		void OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings)override;

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
		void OnContactRemoved(const JPH::SubShapeIDPair& subShapePair)override;

	private:
		/**
		* [EN]
		* Contact state of one body pair. Jolt reports contacts per
		* sub-shape pair, so one body pair can hold several at once.
		*
		* ---------------------------------------------------------------------
		*
		* [JP]
		* 1組のボディペアの接触状態。Jolt は接触をサブシェイプのペアごとに
		* 報告するので、1組のボディペアが同時に複数の接触を持つことがある。
		*/
		struct ContactPair
		{
			/// [EN] Entities represented by the two bodies, the one with the smaller body ID first.
			/// [JP] 2つのボディが表すエンティティ。ボディ ID が小さいほうが first。
			std::pair<EntityID, EntityID> entityIDs_;

			/// [EN] Whether either body is a sensor.
			/// [JP] どちらかのボディがセンサーか。
			Bool isSensor_ = false;

			/// [EN] Number of sub-shape pairs currently in contact.
			/// [JP] 現在接触しているサブシェイプのペア数。
			Uint32 contactCount_ = 0;
		};

		/// [EN] World that receives deferred contact events.
		/// [JP] 遅延接触イベントを受け取る World。
		World* world_ = nullptr;

		/// [EN] Synchronizes callback writes with event dispatch.
		/// [JP] コールバックによる書き込みとイベント通知を同期する。
		std::mutex mutex_;

		/// [EN] Contact events waiting for dispatch on the active World.
		/// [JP] アクティブな World への通知を待つ接触イベント。
		DynamicArray<ContactEvent> pendingEvents_;

		/// [EN] Body pairs in contact, keyed by both body IDs packed into one value, the smaller ID in the upper half.
		/// [JP] 接触中のボディペア。キーは2つのボディ ID を1つの値へまとめたもので、小さいほうの ID を上位側に置く。
		std::unordered_map<Uint64, ContactPair> contactPairs_;

		/// [EN] Body pairs that already queued an enter or stay event during the current step.
		/// [JP] 現在のステップで既に Enter か Stay のイベントを積んだボディペア。
		std::unordered_set<Uint64> notifiedPairs_;

		/// [EN] Body pairs whose last contact was removed, kept while either body sleeps; they exit once both are awake without the contact coming back.
		/// [JP] 最後の接触が削除されたボディペア。どちらかが眠っている間は保持し、両方が起きても接触が戻らなければ Exit する。
		std::unordered_set<Uint64> restingPairs_;
	};
}
