#pragma once
#include <PhysicsEngine/Physics/PhysicsTypes.h>
#include <FoundationEngine/SeedScript.h>

namespace SeedCore
{
	/**
	* [JP]
	* 2つの体をくっつけて、一緒に動くようにする部品。
	*
	* @brief 体をくっつける部品
	*
	* [EN]
	* A part that glues two bodies so they move together.
	*
	* @brief Glues two bodies
	*/
	class SEEDCORE_API FixedJoint :public SeedScript
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
	REGISTER_COMPONENT(FixedJoint, "Physics/Joint");
}
