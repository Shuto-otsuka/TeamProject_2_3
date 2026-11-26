#include <PhysicsEngine/Joint/HingeJoint.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Rigidbody/Rigidbody.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/World/WorldAccessor.h>

namespace SeedCore
{
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
	void HingeJoint::OnStart()
	{
		if (!enabled_)
		{
			return;
		}

		Actor actor = GetActor();

		/// [JP] 自分の Rigidbody がなければつなげない
		/// [EN] Nothing can be joined without our own Rigidbody
		Rigidbody* selfBody = actor.GetComponent<Rigidbody>();
		if (!selfBody)
		{
			SC_LOG_WARNING("HingeJoint: 同じアクターに Rigidbody がありません。");
			return;
		}

		/// [JP] 相手のアクターから Rigidbody を探す。なければ世界につなぐ
		/// [EN] Find the other actor's Rigidbody, or join to the world if there is none
		ScBodyID connectedBodyID = ScInvalidBodyID;
		if (connectedActor_ != 0)
		{
			Actor connected = WorldAccessor::Find(GetWorld(), connectedActor_);
			Rigidbody* connectedBody = connected ? connected.GetComponent<Rigidbody>() : nullptr;
			if (connectedBody)
			{
				connectedBodyID = connectedBody->BodyID();
			}
		}

		/// [JP] 設定をまとめて、つなぎを作る
		/// [EN] Gather the settings and make the joint
		HingeJointDesc desc;
		desc.anchor_ = anchor_;
		desc.axis_ = axis_;
		desc.useLimits_ = useLimits_;
		desc.minAngle_ = minAngle_;
		desc.maxAngle_ = maxAngle_;

		handle_ = actor.GetService<Physics>()->CreateHingeJoint(selfBody->BodyID(), connectedBodyID, desc);
	}

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
	void HingeJoint::OnDestroy()
	{
		if (handle_.exists())
		{
			GetActor().GetService<Physics>()->DestroyJoint(handle_);
			handle_ = ScConstraintHandle::null();
		}
	}
}
