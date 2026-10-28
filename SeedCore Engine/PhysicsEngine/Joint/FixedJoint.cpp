#include <PhysicsEngine/Joint/FixedJoint.h>
#include <PhysicsEngine/Physics/Physics.h>
#include <PhysicsEngine/Rigidbody/Rigidbody.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/Log/Warning.h>
#include <FoundationEngine/World/WorldAccessor.h>

namespace SeedCore
{
	/**
	* [EN]
	* Creates the fixed constraint between the available bodies.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 利用可能なボディ間に固定拘束を生成する。
	*/
	void FixedJoint::OnStart()
	{
		if (!enabled_)
		{
			return;
		}

		Actor actor = GetActor();

		/// [EN] The owning actor must supply the first rigid body.
		/// [JP] 所有アクターは1つ目の剛体を持つ必要がある。
		Rigidbody* selfBody = actor.GetComponent<Rigidbody>();
		if (!selfBody)
		{
			SC_LOG_WARNING("FixedJoint: 同じアクターに Rigidbody がありません。");
			return;
		}

		/// [EN] Resolve the optional connected actor; an invalid body ID represents the world.
		/// [JP] 任意の接続先アクターを解決する。無効なボディ ID はワールドを表す。
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

		FixedJointDesc desc;

		handle_ = actor.GetService<Physics>()->CreateFixedJoint(selfBody->BodyID(), connectedBodyID, desc);
	}

	/**
	* [EN]
	* Destroys the created constraint.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* 生成した拘束を破棄する。
	*/
	void FixedJoint::OnDestroy()
	{
		if (handle_.exists())
		{
			GetActor().GetService<Physics>()->DestroyJoint(handle_);
			handle_ = ScConstraintHandle::null();
		}
	}
}
