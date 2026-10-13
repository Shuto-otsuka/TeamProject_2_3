#include "GoalSystem.h"

#include <SeedCore/ScComponent.h>
#include <SeedCore/ScPhysics.h>
#include <SeedCore/ScLog.h>

using namespace SeedCore;

void GoalSystem::OnStart()
{
	isTranslationSceneReady_ = false;
}

void GoalSystem::OnTick(float elapsedTime)
{
	World& world = GetWorld();
	Actor actor = GetActor();
	Entity entity = actor.GetEntity();

	Physics& physics = actor.GetPhysics();
	Position* position = world.GetComponent<Position>(entity);
	Rotation* rotation = world.GetComponent<Rotation>(entity);
	DynamicArray<EntityID> entityIDs = world.GetPhysics()->Overlap(OverlapShapeKind::Sphere, Vector3(range_, 0.0f, 0.0f), Transform::Vector(*position), Transform::Quat(*rotation));

	for (EntityID entityID : entityIDs)
	{
		Actor other = world.GetActor(entityID);
		
		if (other.HasTag("Player"))
		{
			isTranslationSceneReady_ = true;
			SC_LOG_NOTICE("遷移だよん");
			break;
		}
	}
}