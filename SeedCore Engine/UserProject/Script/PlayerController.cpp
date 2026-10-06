#include "UserProject/Script/PlayerController.h"
#include"UserProject/Script/BulletController.h"

#include <SeedCore/ScInput.h>
#include<SeedCore/ScPrefab.h>
#include <SeedCore/ScScreen.h>
#include <SeedCore/ScPhysics.h>
#include<SeedCore/ScLog.h>
#include <SeedCore/ScScene.h> 

void PlayerController::OnStart()
{
    SeedCore::World& world = GetWorld();

    cameraBrain = world.GetActor("CameraBrain");

    SeedCore::Entity entity = GetActor().GetEntity();

    position = world.GetComponent<SeedCore::Position>(entity);
    rotation = world.GetComponent<SeedCore::Rotation>(entity);
    scale = world.GetComponent<SeedCore::Scale>(entity);
    myCharacterController = world.GetComponent<SeedCore::CharacterController>(entity);
}

void PlayerController::OnTick(float elapsedTime)
{
    switch (state)
    {
        //ステートごとに更新処理を分岐
    case PlayerController::State::USUALLY:
        //いったん通常ステートのみ
        UpdateUsually(elapsedTime);
        break;
    default:
        break;
    }
}

void PlayerController::OnInspectorGUI()
{
    ImGui::Checkbox("isCoyote", &isCoyote);
    ImGui::InputInt("costGauge", &costGauge);
}

void PlayerController::SubCostGauge(int costGauge)
{
    //盤面から弾が消えたときに呼ばれる
    //コストゲージが減少する
    this->costGauge -= costGauge;
    if (this->costGauge < 0)this->costGauge = 0;
}

void PlayerController::UpdateUsually(float elapsedTime)
{
    //水平加速処理
    UpdateHorizontalAcceleration(elapsedTime);
    //旋回処理
    UpdateTurn(elapsedTime);
    //コヨーテタイム更新
    UpdateCoyoteTime(elapsedTime);
    //ジャンプ入力更新
    UpdateInputJump(elapsedTime);
    //発射更新処理
    UpdateInputShot(elapsedTime);
    //落下判定処理
    UpdateFallJudge(elapsedTime);
}

void PlayerController::UpdateHorizontalAcceleration(float elapsedTime)
{
    //空中かどうかで加速力を変える
    bool isGround = myCharacterController->OnGround();
    if (isGround)
        myCharacterController->acceleration_ = acceleration;
    else
        myCharacterController->acceleration_ = airAcceleration;

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
    //ジャンプ先行入力処理
    if (jumpInputBuffer)
    {
        if (OnGroundOrCoyote())
        {
            //先行入力期間中に地面に着いたらその瞬間にジャンプ
            myCharacterController->jumpPower_ = inputBufferJumpPower;//先行入力時の押された時間を基にジャンプ力をセット
            myCharacterController->Jump();
            jumpInputEnable = false;//ジャンプ不可時間開始
            jumpInputEnableTimer = 0.0f;
            jumpReady = false;
        }

        jumpInputBufferTimer += elapsedTime;
        if (jumpInputBufferTimer >= jumpInputBufferTime)
            jumpInputBuffer = false;//一定時間経つと先行入力取り消し
    }

    if (!jumpInputEnable)
    {
        //ジャンプ入力不可時間
        //ジャンプ直後にもコヨーテタイムが発動して2段ジャンプできるのを防ぐ
        jumpInputEnableTimer += elapsedTime;
        if (jumpInputEnableTimer >= jumpEnableTime)
            jumpInputEnable = true;
        else
            return;//ジャンプ不可時間は以下の処理を行わない
    }

    if (SeedCore::Input::ActionState("Jump", SeedCore::Input::OnPressed))
    {
        //ジャンプキー押された瞬間にタイマーリセット
        jumpInputTimer = 0.0f;
        jumpReady = true;
    }
    else if (SeedCore::Input::ActionState("Jump", SeedCore::Input::IsPressed) && jumpReady)
    {
        //ジャンプキー押してる最中にタイマー経過
        jumpInputTimer += elapsedTime;
        if (jumpInputTimer >= maxJumpInputTime)
        {
            //タイマーが最大を超えたら最大ジャンプ力でジャンプ
            if (OnGroundOrCoyote())//長押しし続けてる場合は空中で先行入力はしない
            {
                Jump(maxJumpPower);
                jumpInputEnable = false;//ジャンプ不可時間開始
                jumpInputEnableTimer = 0.0f;
                jumpReady = false;
            }
            else
            {
                jumpReady = false;
            }
        }
    }
    else if (SeedCore::Input::ActionState("Jump", SeedCore::Input::OnReleased) && jumpReady)
    {
        //ジャンプキーが離されたらジャンプ
        // 
        //最小ジャンプ力と最大ジャンプ力、ジャンプキーが押されていた時間からジャンプ力を計算
        //ジャンプキーが押されていた時間が長いほどジャンプ力を高くする
        float jumpPower = SeedCore::Lerp(minJumpPower, maxJumpPower, (jumpInputTimer / maxJumpInputTime));
        if (OnGroundOrCoyote())
        {
            //地面についていれば通常ジャンプ処理
            Jump(jumpPower);
            jumpInputEnable = false;//ジャンプ不可時間開始
            jumpInputEnableTimer = 0.0f;
            jumpReady = false;
        }
        else
        {
            //地面についていなければ先行入力として保存
            jumpInputBuffer = true;
            inputBufferJumpPower = jumpPower;
            jumpInputBufferTimer = 0.0f;
            jumpReady = false;
        }
    }
}

void PlayerController::Jump(float jumpPower)
{
    //ジャンプ
    myCharacterController->jumpPower_ = jumpPower;//ジャンプ力をキャラクターコントローラーにセット
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

void PlayerController::UpdateTurn(float elapsedTime)
{
    //旋回処理
     
    if (shotReady)
    {
        //ショットチャージ時 照準方向(shotDirection)を向く

        SeedCore::Vector2 cursorScreenPosition = SeedCore::Input::MousePoint();//カーソルのスクリーン座標
        SeedCore::Ray ray = SeedCore::ScreenSpace::ScreenToWorld(cursorScreenPosition);//レイキャスト情報
        SeedCore::RaycastHit hit;
        SeedCore::Vector3 targetPosition;
        Uint32 layerMask = ~(1 << GetActor().Layer());//プレイヤー自身を除くレイヤーマスク

        if (GetActor().GetPhysics().Raycast(ray.origin_, ray.direction_, 10000.0f, hit,layerMask))
        {
            SeedCore::Actor hitActor = GetWorld().GetActor(hit.entityID_);//ヒットしたアクターの取得
            if (!hitActor.HasTag("CanStop") && hit.normal_.y * -1 > 0.99f)
            {
                //止められないオブジェクトの上面だった場合
                //レイと銃口の高さの平面が交わるところをターゲットにする
                SeedCore::Vector3 gunOffset = { 0.0f,bulletOffsetY,0.0f };
                SeedCore::Vector3 gunPosition = SeedCore::Vector3::Transform(gunOffset, GetActor().WorldMatrix());
                targetPosition = RayToPlaneHitPosition(ray.origin_, ray.direction_, SeedCore::Vector3{ 0.0f,1.0f,0.0f },gunPosition.y);
            }
            else
            {
                //止められるオブジェクトもしくは壁などに当たった場合当たった場所をターゲットにする
                targetPosition = hit.position_;
            }
        }
        else
        {
            //なににも当たらなかった場合
            //レイと銃口の高さの平面が交わるところをターゲットにする
            SeedCore::Vector3 gunOffset = { 0.0f,bulletOffsetY,0.0f };
            SeedCore::Vector3 gunPosition = SeedCore::Vector3::Transform(gunOffset, GetActor().WorldMatrix());
            targetPosition = RayToPlaneHitPosition(ray.origin_, ray.direction_, SeedCore::Vector3{ 0.0f,1.0f,0.0f }, gunPosition.y);
        }

        //ターゲットに対してのベクトルをshotDirectionとする
        shotDirection = targetPosition - SeedCore::Transform::Vector(*position);
        shotDirection.Normalize();

        TurnFromDirection(elapsedTime,shotDirection);
    }
    else
    {
        //通常時 移動方向(lookDirection)を向く
        TurnFromDirection(elapsedTime,lookDirection);
    }
}

void PlayerController::TurnFromDirection(float elapsedTime,const SeedCore::Vector3& direction)
{
    //軸とアングルを算出
    SeedCore::Vector3 front = GetActor().WorldMatrix().Forward();//右手系
    front.Normalize();
    float dot = front.Dot(direction);
    if (dot > 0.995f)return;//角度がほぼ0だったら終了
    const float minTurnSpeed = 0.2f;//角度が近づくにつれて旋回スピードを小さくするだけだと最後の方が遅すぎるため最低旋回速度を設ける
    float angle = ((1.0f - dot) + minTurnSpeed) * turnSpeed * elapsedTime;//内積から1フレームで旋回させる角度を計算
    SeedCore::Vector3 cross;
    cross = front.Cross(direction);
    cross.Normalize();

    //軸とアングルから回転分のクォータニオンを作成
    SeedCore::Quaternion quaternion = SeedCore::Quaternion::CreateFromAxisAngle(cross, angle);
    quaternion.Normalize();

    //ベクトルと合成してセット
    front = SeedCore::Vector3::Transform(front, quaternion);
    myCharacterController->ForwardDirection(front);
}

void PlayerController::UpdateInputShot(float elapsedTime)
{
    if (SeedCore::Input::MouseState(SeedCore::Input::MouseButton::Left, SeedCore::Input::OnPressed))
    {
        //発射キー押された瞬間にタイマーリセット
        shotReady = true;
        shotInputTimer = 0.0f;
    }

    if (SeedCore::Input::MouseState(SeedCore::Input::MouseButton::Left, SeedCore::Input::IsPressed) && shotReady)
    {
        //発射キー押されている間タイマー経過
        shotInputTimer += elapsedTime;
    }

    if (SeedCore::Input::MouseState(SeedCore::Input::MouseButton::Left, SeedCore::Input::OnReleased) && shotReady)
    {
        //発射キーが離された瞬間に発射
        Shot();
        shotReady = false;
    }
}

void PlayerController::Shot()
{
    //向く方向を撃つ方向にする
    //撃った後に元の向いていた方向に戻ってしまうのを防ぐため
    lookDirection = shotDirection;

    float chargeTime = std::min(shotInputTimer, maxShotChargeTime);
    float chargeRate = chargeTime / maxShotChargeTime;//チャージ時間からチャージ率を計算

    //コストの計算
    int addCostGauge = static_cast<int>(SeedCore::Lerp(static_cast<float>(minAddCostGauge), static_cast<float>(maxAddCostGauge), chargeRate));
    //コストが100超える場合撃たない
    if (costGauge + addCostGauge > maxCostGauge)return;
    //コスト増加
    costGauge += addCostGauge;

    SeedCore::Actor bullet = SeedCore::Prefab::Spawn(SeedCore::String("Bullet.prefab"));//弾生成
    BulletController* bulletController = GetWorld().GetComponent<BulletController>(bullet.GetEntity());

    SeedCore::Vector3 bulletOffset;
    bulletOffset.y = bulletOffsetY;
    bulletOffset.z = SeedCore::Lerp(minBulletOffsetZ, maxBulletOffsetZ, chargeRate);//埋まり防止のため弾のサイズがでかいほどオフセットを空ける
    //プレイヤー姿勢、弾のローカルオフセットから弾のワールド位置を計算
    SeedCore::Vector3 bulletPosition = SeedCore::Vector3::Transform(bulletOffset, GetActor().WorldMatrix());

    bulletController->SetParam(bulletPosition,shotDirection, chargeRate,addCostGauge);//位置、見てる方向、チャージ率、増加コストを渡す
}

void PlayerController::UpdateFallJudge(float elapsedTime)
{
    if (position->y_ < deadPosY)
    {
        //落下のため死亡
        //とりあえずシーン読み込みなおすだけ
        SeedCore::Scene::Change("Sakatyan.scene");
    }
}

bool PlayerController::OnGroundOrCoyote()
{
    return myCharacterController->OnGround() || isCoyote;
}

const SeedCore::Vector3& PlayerController::RayToPlaneHitPosition(const SeedCore::Vector3& rayOrigin, const SeedCore::Vector3& rayDirection, const SeedCore::Vector3& planeNormal, float planeDistance)
{
    //レイと平面の交点を求める関数
    float dot1 = rayDirection.Dot(planeNormal);
    float dot2 = rayOrigin.Dot(planeNormal);
    float sub = planeDistance - dot2;
    float t = sub / dot1;
    SeedCore::Vector3 hitPosition = { rayOrigin.x + rayDirection.x * t,
                       rayOrigin.y + rayDirection.y * t,
                        rayOrigin.z + rayDirection.z * t };
    return hitPosition;
}
