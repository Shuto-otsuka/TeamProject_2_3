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
    Rotate(elapsedTime);
}

void FirstPersonCameraController::FollowPlayer(float elapsedTime)
{
    //プレイヤーに追従する処理
    SeedCore::Vector3 worldPosition = SeedCore::Vector3::Transform(offset, player.WorldMatrix());
    position->x_ = worldPosition.x;
    position->y_ = worldPosition.y;
    position->z_ = worldPosition.z;
}

void FirstPersonCameraController::Rotate(float elapsedTime)
{
    //カメラのyawだけプレイヤーに合わせる
    //pitchはプレイヤー側から制御される
    float playerYaw = SeedCore::Transform::Euler(*playerRotation).y + DirectX::XM_PI;
    float cameraYaw = SeedCore::Transform::Euler(*rotation).y;
    float angle = playerYaw - cameraYaw;
    SeedCore::Vector3 axis = SeedCore::Vector3(0.0f, 1.0f, 0.0f);
    SeedCore::Quaternion quaternion = SeedCore::Quaternion::CreateFromAxisAngle(axis, angle);
    quaternion = SeedCore::Transform::Quat(*rotation) * quaternion;
    rotation->x_ = quaternion.x;
    rotation->y_ = quaternion.y;
    rotation->z_ = quaternion.z;
    rotation->w_ = quaternion.w;
}
