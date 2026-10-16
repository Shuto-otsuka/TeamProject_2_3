#include "UserProject/Script/FirstPersonCameraController.h"

void FirstPersonCameraController::OnStart()
{
    player = GetWorld().GetActor("Player");

    position = GetWorld().GetComponent<SeedCore::Position>(GetActor().GetEntity());
    rotation = GetWorld().GetComponent<SeedCore::Rotation>(GetActor().GetEntity());
    playerPosition = GetWorld().GetComponent<SeedCore::Position>(player.GetEntity());
    playerRotation = GetWorld().GetComponent<SeedCore::Rotation>(player.GetEntity());
}

void FirstPersonCameraController::OnTick(float elapsedTime)
{
    FollowPlayer(elapsedTime);
}

void FirstPersonCameraController::FollowPlayer(float elapsedTime)
{
    SeedCore::Vector3 worldPosition = SeedCore::Vector3::Transform(offset, player.WorldMatrix());
    position->x_ = worldPosition.x;
    position->y_ = worldPosition.y;
    position->z_ = worldPosition.z;
}
