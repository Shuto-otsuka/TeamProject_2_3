#include "UserProject/Script/PlayerController.h"

#include <SeedCore/ScInput.h>
#include <SeedCore/ScLog.h>  

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
        ImGui::Checkbox("isCoyote", &isCoyote);
        break;
    default:
        break;
    }
}

void PlayerController::UpdateUsually(float elapsedTime)
{
    UpdateHorizontalAcceleration(elapsedTime);
    UpdateCoyoteTime(elapsedTime);
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
    //旋回方向をセット
    SeedCore::Vector3 lookDirection = moveDirection;
    lookDirection.Normalize();
    Turn(lookDirection);
}

void PlayerController::UpdateInputJump(float elapsedTime)
{
    if (!OnGroundOrCoyote())
    {
        //地面についていなければ終了
        jumpInputTimer = 0.0f;
        return;
    }

    if (SeedCore::Input::ActionState("Jump", SeedCore::Input::OnPressed))
    {
        //ジャンプキー押し始めた瞬間にタイマーリセット
        jumpInputTimer = 0.0f;
        jumpReady = true;
    }
    else if (SeedCore::Input::ActionState("Jump", SeedCore::Input::IsPressed))
    {
        //ジャンプキー押してる最中にタイマー経過
        jumpInputTimer += elapsedTime;
        if (jumpInputTimer >= maxJumpInputTime)
        {
            //タイマーが最大を超えたら最大ジャンプ力でジャンプ
            Jump(maxJumpPower);
            jumpReady = false;
        }
    }
    else if (SeedCore::Input::ActionState("Jump", SeedCore::Input::OnReleased) && jumpReady)
    {
        //ジャンプキーが離されたらジャンプ

        //最小ジャンプ力と最大ジャンプ力、ジャンプキーが押されていた時間からジャンプ力を計算
        //ジャンプキーが押されていた時間が長いほどジャンプ力を高くする
        float jumpPower = SeedCore::Lerp(minJumpPower, maxJumpPower, (jumpInputTimer / maxJumpInputTime));
        Jump(jumpPower);
        jumpReady = false;
    }
}

void PlayerController::Jump(float jumpPower)
{
    //ジャンプ
    myCharacterController->jumpPower_ = jumpPower;
    myCharacterController->Jump();
}

void PlayerController::UpdateCoyoteTime(float elapsedTime)
{
    if(isCoyote)
    {
        //コヨーテタイム
        coyoteTimer += elapsedTime;
        if (coyoteTimer >= coyoteTime)
        {
            //コヨーテタイム終了
            isCoyote = false;
        }
    }
    else
    {
        bool isGround = myCharacterController->OnGround();
        if (!isGround && beforeIsGround)
        {
            //足が地面から離れた瞬間にコヨーテタイムに入る
            isCoyote = true;
        }
    }

    //前回の接地判定を保存
    beforeIsGround = myCharacterController->OnGround();
}

void PlayerController::Turn(SeedCore::Vector3 lookDirection)
{
    //旋回処理
    //myCharacterController->ForwardDirection(lookDirection);
}

bool PlayerController::OnGroundOrCoyote()
{
    return myCharacterController->OnGround() || isCoyote;
}
