#include "UserProject/Script/PlayerController.h"

#include <SeedCore/ScInput.h>
#include <SeedCore/ScLog.h>  

void PlayerController::OnStart()
{
    SeedCore::World& world = GetWorld();
    SeedCore::Entity entity = GetActor().GetEntity();

    cameraBrain = world.GetActor("CameraBrain");

    position = world.GetComponent<SeedCore::Position>(entity);
    rotation = world.GetComponent<SeedCore::Rotation>(entity);
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

void PlayerController::OnInspectorGUI()
{
    ImGui::Checkbox("isCoyote", &isCoyote);
}

void PlayerController::UpdateUsually(float elapsedTime)
{
    //水平加速処理
    UpdateHorizontalAcceleration(elapsedTime);
    //旋回処理
    Turn(elapsedTime);
    //コヨーテタイム更新
    UpdateCoyoteTime(elapsedTime);
    //ジャンプ入力更新
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
    if (moveDirection.Length() > 0.001f)
    {
        moveDirection.Normalize();
        lookDirection = moveDirection;
    }
}

void PlayerController::UpdateInputJump(float elapsedTime)
{
    if (!jumpInputEnable)
    {
        //ジャンプ入力不可時間
        //ジャンプ直後にもコヨーテタイムが発動して2段ジャンプできるのを防ぐ
        jumpInputEnableTimer += elapsedTime;
        if (jumpInputEnableTimer >= jumpEnableTime)
            jumpInputEnable = true;
        else
            return;
    }

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
        jumpInputEnable = false;
        jumpInputEnableTimer = 0.0f;
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
            coyoteTimer = 0.0f;
        }
    }

    //前回の接地判定を保存
    beforeIsGround = myCharacterController->OnGround();
}

void PlayerController::Turn(float elapsedTime)
{
    //旋回処理
    //移動方向(lookDirection)を向く

    //軸とアングルを算出
    SeedCore::Vector3 front = GetActor().WorldMatrix().Forward();//右手系
    front.Normalize();
    float dot = front.Dot(lookDirection);
    if (dot > 0.995f)return;//角度がほぼ0だったら終了
    const float minTurnSpeed = 0.2f;//角度が近づくにつれて旋回スピードを小さくするだけだと最後の方が遅すぎるため最低旋回速度を設ける
    float angle = ((1.0f - dot) + minTurnSpeed) * turnSpeed * elapsedTime;//内積から1フレームで旋回させる角度を計算
    SeedCore::Vector3 cross;
    cross = front.Cross(lookDirection);
    cross.Normalize();
    
    //軸とアングルから回転分のクォータニオンを作成
    SeedCore::Quaternion quaternion = SeedCore::Quaternion::CreateFromAxisAngle(cross, angle);
    quaternion.Normalize();
   
    //ベクトルと合成してセット
    front = SeedCore::Vector3::Transform(front, quaternion);
    myCharacterController->ForwardDirection(front);
}

bool PlayerController::OnGroundOrCoyote()
{
    return myCharacterController->OnGround() || isCoyote;
}
