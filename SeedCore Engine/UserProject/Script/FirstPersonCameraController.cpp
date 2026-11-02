#include "UserProject/Script/FirstPersonCameraController.h"
#include "UserProject/Script/PlayerController.h"
#include<SeedCore/ScInput.h>

void FirstPersonCameraController::OnStart()
{
    player = GetWorld().GetActor("Player");

    position = GetActor().GetComponent<SeedCore::Position>();
    rotation = GetActor().GetComponent<SeedCore::Rotation>();
    playerPosition = player.GetComponent<SeedCore::Position>();
    playerRotation = player.GetComponent<SeedCore::Rotation>();
    playerController = player.GetComponent<PlayerController>();
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
    //pitchはカーソルの移動量から動かす
    yaw = SeedCore::Transform::Euler(*playerRotation).y + DirectX::XM_PI;
 
    SeedCore::Vector2 cursorMove = SeedCore::Input::MouseMotion();
    pitch -= cursorMove.y * playerController->firstPersonSensitivity;
    float minPitch = DirectX::XMConvertToRadians(playerController->firstPersonMinPitch);
    float maxPitch = DirectX::XMConvertToRadians(playerController->firstPersonMaxPitch);
    pitch = std::clamp(pitch,minPitch, maxPitch);

    SeedCore::Quaternion quaternion = SeedCore::Quaternion::CreateFromYawPitchRoll(yaw,pitch,0);
    quaternion.Normalize();

    rotation->x_ = quaternion.x;
    rotation->y_ = quaternion.y;
    rotation->z_ = quaternion.z;
    rotation->w_ = quaternion.w;
}
