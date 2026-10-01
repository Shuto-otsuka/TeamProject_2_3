#include "UserProject/Script/PlayerController.h"

#include <SeedCore/ScPhysics.h>  
#include <SeedCore/ScInput.h>

void PlayerController::OnStart()
{
    SeedCore::World& world = GetWorld();
    SeedCore::Entity entity = GetActor().GetEntity();

    position = world.GetComponent<SeedCore::Position>(entity);
    myRigid = world.GetComponent<SeedCore::Rigidbody>(entity);
    myVelocity = world.GetComponent<SeedCore::Velocity>(entity);
}

void PlayerController::OnTick(float elapsedTime)
{
    switch (state)
    {
        //ステートごとに更新処理を分岐
    case PlayerController::State::USUALLY:
        UpdateUsually(elapsedTime);
        break;
    default:
        break;
    }
}

void PlayerController::UpdateUsually(float elapsedTime)
{
    //移動入力情報を取得
    SeedCore::Vector2 inputDirection = SeedCore::Input::ActionAxis("Move");
    //入力情報から移動方向を算出
    SeedCore::Vector3 moveDirection = { inputDirection.x,0.0f,inputDirection.y };
    moveDirection.Normalize();
}
