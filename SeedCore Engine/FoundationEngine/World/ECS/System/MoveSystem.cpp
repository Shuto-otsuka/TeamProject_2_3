#include <FoundationEngine/World/ECS/System/MoveSystem.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>
#include <FoundationEngine/World/ECS/Component/Velocity.h>

namespace SeedCore
{
	/**
	* [EN]
	* Advances every active actor by deltaTime: LinearVelocity is added
	* onto Position, and AngularVelocity turns Rotation about its axis.
	* Inactive actors, and actors whose velocity is owned by a physics
	* body, keep their Position and Rotation unchanged.
	*
	* ---------------------------------------------------------------------
	*
	* [JP]
	* アクティブな全 actor を deltaTime 分だけ進める。LinearVelocity は
	* Position へ加算し、AngularVelocity はその軸まわりに Rotation を回す。
	* 非アクティブな actor と、速度を物理ボディが管理している actor の
	* Position と Rotation は変えない。
	*/
	void MoveSystem::Execute(World& world, Float deltaTime)
	{
		Query<Read<LinearVelocity>, Write<Position>> linearQuery(world);

		linearQuery.ForEach([&](EntityID entityID, const LinearVelocity& velocity, Position& position)
			{
				Actor actor = world.GetActor(entityID);
				if (actor && !actor.Active())
				{
					return;
				}

				/// [EN] A velocity owned by a physics body already moves Position through the body's pose.
				/// [JP] 物理ボディが管理する速度は、ボディの姿勢を通して既に Position を動かしている。
				if (velocity.simulated_)
				{
					return;
				}

				position.x_ += velocity.x_ * deltaTime;
				position.y_ += velocity.y_ * deltaTime;
				position.z_ += velocity.z_ * deltaTime;
			});

		Query<Read<AngularVelocity>, Write<Rotation>> angularQuery(world);

		angularQuery.ForEach([&](EntityID entityID, const AngularVelocity& velocity, Rotation& rotation)
			{
				Actor actor = world.GetActor(entityID);
				if (actor && !actor.Active())
				{
					return;
				}

				/// [EN] An angular velocity owned by a physics body already turns Rotation through the body's pose.
				/// [JP] 物理ボディが管理する角速度は、ボディの姿勢を通して既に Rotation を回している。
				if (velocity.simulated_)
				{
					return;
				}

				/// [EN] The angular velocity's direction is the axis and its length the speed, so this step turns by length times deltaTime about that axis.
				/// [JP] 角速度の向きが回転軸、長さが回転の速さなので、このステップではその軸まわりに長さ × deltaTime だけ回す。
				Vector3 angularVelocity = Vector3(velocity.x_, velocity.y_, velocity.z_);
				Float speed = angularVelocity.Length();
				if (speed <= 0.0f)
				{
					return;
				}

				/// [EN] The axis is in the same space as Position, so the turn is applied after the current rotation; renormalizing keeps rounding from building up frame after frame.
				/// [JP] 軸は Position と同じ空間なので、回転は今の回転の後に掛ける。毎フレームの丸め誤差が積み重ならないよう正規化し直す。
				Quaternion turn = Quaternion::CreateFromAxisAngle(angularVelocity / speed, speed * deltaTime);
				Quaternion turned = Transform::Quat(rotation) * turn;
				turned.Normalize();
				Transform::Quat(rotation, turned);
			});
	}
}
