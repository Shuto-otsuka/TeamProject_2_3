#include "GoalSystem.h"

#include <SeedCore/ScPhysics.h>
#include <SeedCore/ScLog.h>

using namespace SeedCore;

void GoalSystem::OnStart()
{
	isGoal_ = false;
	isDecieded_ = false;

	World& world = GetWorld();

	resultActor_ = world.GetActor("ResultUI");
	resultActor_.Active(false);

	world.Timer().TimeScale(1.0f);
}

void GoalSystem::OnTick(float elapsedTime)
{
	World& world = GetWorld();
	Actor actor = GetActor();
	Entity entity = actor.GetEntity();

	if (!isGoal_)
	{
		Physics& physics = actor.GetPhysics();
		Position* position = world.GetComponent<Position>(entity);
		Rotation* rotation = world.GetComponent<Rotation>(entity);
		DynamicArray<EntityID> entityIDs = world.GetPhysics()->Overlap(OverlapShapeKind::Sphere, Vector3(range_, 0.0f, 0.0f), Transform::Vector(*position), Transform::Quat(*rotation));

		for (EntityID entityID : entityIDs)
		{
			Actor other = world.GetActor(entityID);

			if (other.HasTag("Player"))
			{
				isGoal_ = true;
				resultActor_.Active(true);
				break;
			}
		}
	}
	else
	{
		world.Timer().TimeScale(0.0f);
	}
}

void GoalSystem::OnEditorTick(float elapsedTime)
{
	Position* position = GetWorld().GetComponent<Position>(GetActor().GetEntity());
	GetActor().GetDebugDraw().Sphere(Transform::Vector(*position), 0.5f, Color(1.0f, 0.0f, 0.0f, 0.5f));
} 