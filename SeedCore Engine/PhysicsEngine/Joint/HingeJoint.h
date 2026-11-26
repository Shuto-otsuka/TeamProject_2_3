#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [JP]
	* ドアの蝶番のように、軸の周りだけ回れるようにつなぐ部品。
	*
	* @brief 軸の周りで回るようにつなぐ部品
	*
	* [EN]
	* A part that joins two bodies so they can only turn around an axis, like a door hinge.
	*
	* @brief Joins bodies on a hinge
	*/
	class SEEDCORE_API HingeJoint :public SeedScript
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

		/// [JP] つなぐ点（自分から見た位置）
		/// [EN] The joining point, seen from this actor
		SC_REFLECTION_FIELD_EX("アンカー(ローカル)")
		Vector3 anchor_ = { 0.0f, 0.0f, 0.0f };

		/// [JP] 動ける向き（自分から見た向き）
		/// [EN] The direction it can move in, seen from this actor
		SC_REFLECTION_FIELD_EX("ヒンジ軸(ローカル)")
		Vector3 axis_ = { 0.0f, 1.0f, 0.0f };

		/// [JP] 動ける範囲を決めるか
		/// [EN] Whether to limit how far it can move
		SC_REFLECTION_FIELD_EX("角度制限を使う")
		Bool useLimits_ = false;

		/// [JP] 回れる一番小さい角度（度）
		/// [EN] The smallest angle it can turn to, in degrees
		SC_REFLECTION_FIELD_CONDITION(useLimits_)
		SC_REFLECTION_CLAMPED_EX("最小角(度)", -180.0f, 0.0f)
		Float minAngle_ = 0.0f;

		/// [JP] 回れる一番大きい角度（度）
		/// [EN] The largest angle it can turn to, in degrees
		SC_REFLECTION_FIELD_CONDITION(useLimits_)
		SC_REFLECTION_CLAMPED_EX("最大角(度)", 0.0f, 180.0f)
		Float maxAngle_ = 0.0f;

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
	REGISTER_COMPONENT(HingeJoint, "Physics/Joint");
}
