#include "UserProject/Script/Test.h"

void Test::OnStart()
{
    SeedCore::World& world = GetWorld();
    SeedCore::Entity entity = GetActor().GetEntity();

    position = world.GetComponent<SeedCore::Position>(entity);
}

void Test::OnTick(float elapsedTime)
{
    position->z_ += speed * elapsedTime;
}
