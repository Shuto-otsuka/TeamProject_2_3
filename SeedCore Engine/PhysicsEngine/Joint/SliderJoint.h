#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [JP]
	* 決めた向きにだけ滑って動けるようにつなぐ部品。
	*
	* @brief 決めた向きに滑るようにつなぐ部品
	*
	* [EN]
	* A part that joins two bodies so they can only slide along one direction.
	*
	* @brief Joins bodies on a slider
	*/
	class SEEDCORE_API SliderJoint :public SeedScript
	{
	public:
		/// [JP] つなぐかどうか
		/// [EN] Whether to join at all
		SC_REFLECTION_FIELD_EX("有効")
		Bool enabled_ = true;

		/// [JP] つなぐ相手のアクター。0 なら世界につなぐ
		/// [EN] The actor to join to; 0 joins to the world
		SC_PAYLOAD_FIELD_EX("接続先アクター", Actor)
		Uint32 connectedActor_ = 0;

		/// [JP] 動ける向き（自分から見た向き）
		/// [EN] The direction it can move in, seen from this actor
		SC_REFLECTION_FIELD_EX("スライド軸(ローカル)")
		Vector3 axis_ = { 1.0f, 0.0f, 0.0f };

		/// [JP] 動ける範囲を決めるか
		/// [EN] Whether to limit how far it can move
		SC_REFLECTION_FIELD_EX("可動範囲制限を使う")
		Bool useLimits_ = false;

		/// [JP] 一番近づける距離
		/// [EN] The closest it can get
		SC_REFLECTION_FIELD_CONDITION(useLimits_)
		SC_REFLECTION_CLAMPED_EX("最小距離", -1000.0f, 0.0f)
		Float minDistance_ = 0.0f;

		/// [JP] 一番離れられる距離
		/// [EN] The farthest it can get
		SC_REFLECTION_FIELD_CONDITION(useLimits_)
		SC_REFLECTION_CLAMPED_EX("最大距離", 0.0f, 1000.0f)
		Float maxDistance_ = 0.0f;

	public:
		/**
		* [JP]
		* 同じアクターの Rigidbody と相手をつなぐ。
		*
		* @brief つなぐ
		* @note  同じアクターに Rigidbody が要る
		*
		* [EN]
		* Joins this actor's Rigidbody to the other one.
		*
		* @brief Makes the joint
		* @note  Needs a Rigidbody on the same actor
		*/
		void OnStart();

		/**
		* [JP]
		* つなぎを外す。
		*
		* @brief つなぎを外す
		*
		* [EN]
		* Removes the joint.
		*
		* @brief Removes the joint
		*/
		void OnDestroy();

	private:
		/// [JP] 物理エンジンの中のつなぎの目印
		/// [EN] The handle of the joint inside the physics engine
		ScConstraintHandle handle_;
	};
	REGISTER_COMPONENT(SliderJoint, "Physics/Joint");
}
