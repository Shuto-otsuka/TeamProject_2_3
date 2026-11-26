#include <FoundationEngine/World/ECS/System/TransformSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/WorldAccessor.h>
#include <FoundationEngine/World/Actor/Actor.h>
#include <FoundationEngine/World/ECS/Component/Position.h>
#include <FoundationEngine/World/ECS/Component/Rotation.h>
#include <FoundationEngine/World/ECS/Component/Scale.h>
#include <FoundationEngine/World/ECS/Component/Transform.h>

namespace SeedCore
{
	void TransformSystem::Execute(World& world)
	{
		ActorRegistry& actors = WorldAccessor::Actors(world);
		for (EntityID root = actors.Root(); root != EntityID{}; root = actors.Record(root)->nextSibling_)
		{
			UpdateActor(WorldAccessor::Make(world, root), Matrix::Identity, world);
		}
	}

	void TransformSystem::UpdateActor(Actor actor, const Matrix& parentMatrix, World& world)
	{
		Position* position = actor.GetComponent<Position>();
		Rotation* rotation = actor.GetComponent<Rotation>();
		Scale* scale = actor.GetComponent<Scale>();

		Matrix local = Matrix::Identity;

		if (scale)
		{
			local *= Matrix::CreateScale(scale->x_, scale->y_, scale->z_);
		}

		if (rotation)
		{
			local *= Matrix::CreateFromQuaternion(Transform::Quat(*rotation));
		}

		if (position)
		{
			local *= Matrix::CreateTranslation(position->x_, position->y_, position->z_);
		}

		Matrix worldMatrix = local * parentMatrix;
		actor.WorldMatrix(worldMatrix);

		for (const Actor& child : actor.Children())
		{
			UpdateActor(child, worldMatrix, world);
		}
	}
}
