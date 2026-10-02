#include "UserProject/Script/PlayerController.h"

#include <SeedCore/ScInput.h>

void PlayerController::OnStart()
{
    SeedCore::World& world = GetWorld();
    SeedCore::Entity entity = GetActor().GetEntity();

    cameraBrain = world.GetActor("CameraBrain");

    position = world.GetComponent<SeedCore::Position>(entity);
    myCharacterController = world.GetComponent<SeedCore::CharacterController>(entity);
}

void PlayerController::OnTick(float elapsedTime)
{
    switch (state)
    {
        //ステートごとに更新処理を分岐
    case PlayerController::State::USUALLY:
        //いったん通常ステート
        UpdateUsually(elapsedTime);
        break;
    default:
        break;
    }
}

void PlayerController::UpdateUsually(float elapsedTime)
{
    UpdateHorizontalAcceleration(elapsedTime); 
    UpdateInputJump(elapsedTime);
}

void PlayerController::UpdateHorizontalAcceleration(float elapsedTime)
{
    //移動入力情報を取得
    SeedCore::Vector2 inputDirection = SeedCore::Input::ActionAxis("Move");
    inputDirection.Normalize();
    //カメラの前方向、右方向ベクトルを取得
    SeedCore::Vector3 cameraFront;
    SeedCore::Vector3 cameraRight;
    cameraFront = cameraBrain.WorldMatrix().Backward();//右手系;
    cameraFront.y = 0.0f;//Y軸は必要ない
    cameraFront.Normalize();
    cameraRight = cameraBrain.WorldMatrix().Left();//右手系;
    cameraRight.y = 0.0f;
    cameraRight.Normalize();
    //入力情報から移動方向を算出
    SeedCore::Vector3 moveDirection;
    moveDirection.x = cameraRight.x * inputDirection.x + cameraFront.x * inputDirection.y;
    moveDirection.y = 0.0f;
    moveDirection.z = cameraRight.z * inputDirection.x + cameraFront.z * inputDirection.y;
    //移動方向をキャラクターコントローラーにセット
    myCharacterController->MoveDirection(moveDirection);
}

void PlayerController::UpdateInputJump(float elapsedTime)
{
    if (!SeedCore::Input::ActionState("Jump", SeedCore::Input::OnPressed))return;
    if (!myCharacterController->OnGround())return;

    //ジャンプキー押された・地面に着いてる
    Jump();
}

void PlayerController::Jump()
{
    //ジャンプ
    myCharacterController->Jump();
}
