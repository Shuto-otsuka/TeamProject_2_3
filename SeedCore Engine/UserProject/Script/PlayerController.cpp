#include "UserProject/Script/PlayerController.h"
#include"UserProject/Script/BulletController.h"
#include"UserProject/Script/StopController.h"

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
    velocity = world.GetComponent<SeedCore::Velocity>(entity);
    myCharacterController = world.GetComponent<SeedCore::CharacterController>(entity);

    defaultGravity = myCharacterController->gravityScale_;
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
    //弾全削除処理
    UpdateAllBulletRemove(elapsedTime);
    //落下判定処理
    UpdateFallJudge(elapsedTime);
    //やり直し処理
    UpdateInputReset(elapsedTime);
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
    if (isJumpKeyReleaseWait)
    {
        //上昇中

        jumpInputTimer += elapsedTime;
        if (jumpInputTimer <= maxJumpInputTime)
        {
            //この時間中にジャンプキーを離すと重力が大きくなる
            //これによってジャンプキーを押している時間が長いほどジャンプ力が高くなる
            if (!SeedCore::Input::ActionState("Jump", SeedCore::Input::IsPressed))
            {
                myCharacterController->gravityScale_ = defaultGravity * jumpGravityScaler;
                isJumpKeyReleaseWait = false;//離し待ち終了
            }
        }
        else
        {
            //時間経過で離し待ち終了
            isJumpKeyReleaseWait = false;
        }
    }

    //着地時に大きくした重力を元に戻す
    if (myCharacterController->OnGround())
        myCharacterController->gravityScale_ = defaultGravity;

    //ジャンプ先行入力処理
    if (jumpInputBuffer)
    {
        if (OnGroundOrCoyote())
        {
            //先行入力期間中に地面に着いたらその瞬間にジャンプ
            Jump();
            isJumpKeyReleaseWait = true;//ジャンプキー離し入力待ち
            jumpInputTimer = 0.0f;
            jumpInputEnable = false;//ジャンプ不可時間開始
            jumpInputEnableTimer = 0.0f;
            //同時にジャンプを呼ぶのを防ぐため終了
            return;
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
        if (OnGroundOrCoyote())
        {
            //地面に着いてる、もしくはコヨーテタイムならジャンプ
            Jump();
            isJumpKeyReleaseWait = true;//ジャンプキー離し入力待ち
            jumpInputTimer = 0.0f;
            jumpInputEnable = false;//ジャンプ不可時間開始
            jumpInputEnableTimer = 0.0f;
        }
        else
        {
            //地面に着いていなければ先行入力として保存
            jumpInputBuffer = true;
            jumpInputBufferTimer = 0.0f;
        }
    }
}

void PlayerController::Jump()
{
    //ジャンプ
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
        Uint32 layerMask = ~(1 << GetActor().Layer());//プレイヤー自身を除くレイヤーマスク

        //銃口の高さを求める
        SeedCore::Vector3 gunOffset = { 0.0f,bulletOffsetY,0.0f };
        SeedCore::Vector3 gunPosition = SeedCore::Vector3::Transform(gunOffset, GetActor().WorldMatrix());

        if (GetActor().GetPhysics().Raycast(ray.origin_, ray.direction_, 10000.0f, hit,layerMask))
        {
            SeedCore::Actor hitActor = GetWorld().GetActor(hit.entityID_);//ヒットしたアクターの取得
            if (!hitActor.HasTag("CanStop") && hit.normal_.y > 0.99f)
            {
                //止められないオブジェクトの上面だった場合
                //レイと銃口の高さの平面が交わるところをターゲットにする
                bulletTargetPosition = RayToPlaneHitPosition(ray.origin_, ray.direction_, SeedCore::Vector3{ 0.0f,1.0f,0.0f },gunPosition.y);

                //ターゲット位置を最大飛距離まで飛んだ時の位置にする
                SeedCore::Vector3 direction = bulletTargetPosition - gunPosition;
                direction.Normalize();
                direction *= bulletRemoveDistance;
                bulletTargetPosition += direction;
            }
            else
            {
                //止められるオブジェクトもしくは壁などに当たった場合当たった場所をターゲットにする
                bulletTargetPosition = hit.position_;
            }
        }
        else
        {
            //なににも当たらなかった場合
            //レイと銃口の高さの平面が交わるところをターゲットにする
            bulletTargetPosition = RayToPlaneHitPosition(ray.origin_, ray.direction_, SeedCore::Vector3{ 0.0f,1.0f,0.0f }, gunPosition.y);

            //ターゲット位置を最大飛距離まで飛んだ時の位置にする
            SeedCore::Vector3 direction = bulletTargetPosition - gunPosition;
            direction.Normalize();
            direction *= bulletRemoveDistance;
            bulletTargetPosition += direction;
        }

        //ターゲットに対してのベクトルをshotDirectionとする
        shotDirection = bulletTargetPosition - gunPosition;
        shotDirection.Normalize();

        //撃つ方向の高さを無視して見る方向として渡す
        SeedCore::Vector3 sD = shotDirection;
        sD.y = 0.0f;
        sD.Normalize();
        TurnFromDirection(elapsedTime,sD);
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

        //チャージ時間からチャージ率を計算
        float chargeTime = std::min(shotInputTimer, maxShotChargeTime);
        shotChargeRate = chargeTime / maxShotChargeTime;

        //チャージ率からコストを計算
        costGaugeAdd = static_cast<int>(SeedCore::Lerp(static_cast<float>(minAddCostGauge), static_cast<float>(maxAddCostGauge), shotChargeRate));

        SeedCore::Vector3 bulletOffset;
        bulletOffset.y = bulletOffsetY;
        bulletOffset.z = SeedCore::Lerp(minBulletOffsetZ, maxBulletOffsetZ, shotChargeRate);//埋まり防止のため弾のサイズがでかいほどオフセットを空ける
        //プレイヤー姿勢、弾のローカルオフセットから弾のワールド位置を計算
        bulletPosition = SeedCore::Vector3::Transform(bulletOffset, GetActor().WorldMatrix());
    }

    if (SeedCore::Input::MouseState(SeedCore::Input::MouseButton::Left, SeedCore::Input::OnReleased) && shotReady)
    {
        //発射キーが離された瞬間に発射
        Shot();

        //ショットチャージ中のパラメータをリセット
        shotChargeRate = 0.0f;
        costGaugeAdd = 0;

        shotReady = false;
    }
}

void PlayerController::Shot()
{
    //向く方向を撃つ方向にする
    //撃った後に元の向いていた方向に戻ってしまうのを防ぐため
    lookDirection = shotDirection;

    //コストが100超える場合撃たない
    if (costGauge + costGaugeAdd > maxCostGauge)return;
    //コスト増加
    costGauge += costGaugeAdd;

    SeedCore::Actor bullet = SeedCore::Prefab::Spawn(SeedCore::String("Bullet.prefab"));//弾生成
    BulletController* bulletController = GetWorld().GetComponent<BulletController>(bullet.GetEntity());

    bulletController->SetParam(bulletPosition,shotDirection, shotChargeRate,costGaugeAdd);//位置、見てる方向、チャージ率、増加コストを渡す
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

void PlayerController::UpdateAllBulletRemove(float elapsedTime)
{
    if (!SeedCore::Input::KeyState(SeedCore::Input::Key::R, SeedCore::Input::OnPressed))return;//Rキー押された瞬間じゃなければ終了

    //アクターを走査
    for (auto& actor : GetWorld().GetActors())
    {
        //全ての弾を削除
        if (actor.HasTag("Bullet"))
        {
            SeedCore::Entity entity = actor.GetEntity();
            BulletController* bulletController = GetWorld().GetComponent<BulletController>(entity);
            bulletController->Remove();
        }

        //全てのギミックを動かす
        if (actor.LayerName() == "Gimmick")
        {
            SeedCore::Entity entity = actor.GetEntity();
            StopController* stopController = GetWorld().GetComponent<StopController>(entity);
            stopController->Move();
        }
    }
}

void PlayerController::UpdateInputReset(float elapsedTime)
{
    if (SeedCore::Input::KeyState(SeedCore::Input::Key::R, SeedCore::Input::IsPressed))
    {
        //Rキーが押されている間タイマー更新
        resetInputTimer += elapsedTime;
        //一定時間でやり直し
        if(resetInputTimer >= resetInputTime)
            SeedCore::Scene::Change("Sakatyan.scene");
    }
   
    if (SeedCore::Input::KeyState(SeedCore::Input::Key::R, SeedCore::Input::OnReleased))
    {
        //タイマーリセット
        resetInputTimer = 0.0f;
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
