#include "UserProject/Script/BulletController.h"
#include"UserProject/Script/PlayerController.h"
#include"UserProject/Script/StopController.h"
#include<SeedCore/ScLog.h>
#include<SeedCore/ScPrefab.h>
#include<SeedCore/ScScreen.h>

void BulletController::OnStart()
{
    player = GetWorld().GetActor("Player");
    playerController = player.GetComponent<PlayerController>();
    linearVelocity = GetActor().GetComponent<SeedCore::LinearVelocity>();
    angularVelocity = GetActor().GetComponent<SeedCore::AngularVelocity>();
}

void BulletController::OnTick(float elapsedTime)
{
    //移動処理
    Move(elapsedTime);
    //生存時間が終了したら削除
    UpdateAliveTime(elapsedTime);
    //ストップタイマー更新処理
    UpdateStopTimer(elapsedTime);
}

void BulletController::OnTriggerEnter(SeedCore::Actor other)
{
    //通常時じゃなければ終了
    if (state != State::NORMAL)return;

    //ヒットしたアクターの取得
    SeedCore::Actor hitActor = other;
    SeedCore::Name* hitActorName = other.GetComponent<SeedCore::Name>();

    if (hitActor.HasTag("GravityArea"))
    {
        //重力エリアならフラグONにして終了
        isGravityArea = true;
        return;
    }

    if (hitActor.HasTag("NoScaffoldArea"))
    {
        //足場作れないエリアならフラグONにして終了
        isNoScaffoldArea = true;
        return;
    }

    //プレイヤー、もしくはエリア(トリガー)なら終了
    if (hitActor.LayerName() == "Player" || hitActor.LayerName() == "Area")return;

    if (!hitActor.HasTag("CanStop"))
    {
        //止められるオブジェクトじゃない
        SC_LOG_NOTICE("止められないやつに当たった");

        if (isGravityArea)
        {
            //重力エリアであれば停止
            Stop();
            SC_LOG_NOTICE("うあああああ");
        }
        else
        {
            //重力エリアじゃなければ消して終了
            Remove();
        }
        return;
    }

    if (hitActor.HasTag("Bullet"))
    {
        //弾に当たった場合
        SC_LOG_NOTICE("弾に当たった");

        BulletController* hitBulletController = other.GetComponent<BulletController>();
        //相手の弾のステートによって処理を変える
        switch (hitBulletController->GetState())
        {
        case State::NORMAL:
        {
            if (isNoScaffoldArea)
            {
                //足場作れないエリアならどちらも消す
                Remove();
                hitBulletController->Remove();
            }
            else
            {
                //自分の方が長く生きてたら終了
                //後で撃ったやつが先撃ってたやつに対して効果を発動するため
                if (aliveTimer > hitBulletController->GetAliveTimer())return;

                hitBulletController->Stop();//停止
                Remove();//自分は削除
            }
            break;
        }
        case State::STOP:
        {
            //既に停止中の弾に当たった

            if (isGravityArea)
            {
                //重力エリアであればRigidbodyをDynamicにして停止
                Stop();
            }
            else
            {
                //重力エリアじゃなければ消して終了
                Remove();
            }
            break;
        }
        case State::STICK:
        {
            if (isGravityArea)
            {
                //重力エリアであればRigidbodyをDynamicにして停止
                Stop();
            }
            else
            {
                //当たった弾が止めている相手のギミックを止める
                float stopTime = aliveTime - aliveTimer;
                hitBulletController->GetNowStick()->Stop(stopTime);
                //自分は動きを止めてギミックにくっついたという判定にする
                state = State::STICK;
                //止めている相手を保存
                nowStick = hitBulletController->GetNowStick();
            }
            break; 
        }
        default:
            break;
        }
    }
    else
    {
        //ギミック停止処理
        SC_LOG_NOTICE("ギミックにあたった");

        //停止を制御するコンポネント
        StopController* stopController = other.GetComponent<StopController>();
        //止める時間を残りの生存時間にする
        float stopTime = aliveTime - aliveTimer;
        //止める
        stopController->Stop(stopTime);

        //自分は動きを止めてギミックにくっついたという判定にする
        state = State::STICK;
        //止めている相手を保存
        nowStick = stopController;
    }
}

void BulletController::OnTriggerExit(SeedCore::Actor other)
{
    //ヒットしたアクターの取得
    SeedCore::Actor hitActor = other;

    if (hitActor.HasTag("GravityArea"))
    {
        //重力エリアならフラグOFF
        isGravityArea = false;
        return;
    }

    if (hitActor.HasTag("NoScaffoldArea"))
    {
        //足場作れないエリアならフラグOFF
        isNoScaffoldArea = false;
        return;
    }
}

void BulletController::SetParam(const SeedCore::Vector3& startPosition,const SeedCore::Vector3& moveDirection, float chargeRate,int cost)
{
    //発射されたタイミングでパラメータをセット

    SeedCore::World& world = GetWorld();

    scale = GetActor().GetComponent<SeedCore::Scale>();
    position = GetActor().GetComponent<SeedCore::Position>();
    rotation = GetActor().GetComponent<SeedCore::Rotation>();
    rigidbody = GetActor().GetComponent<SeedCore::Rigidbody>();

    //初期位置をセット
    this->startPosition = startPosition;
    position->x_ = startPosition.x;
    position->y_ = startPosition.y;
    position->z_ = startPosition.z;

    //進行方向をセット
    this->moveDirection = moveDirection;
    //回転方向をセット
    turnAxis.Normalize();

    //チャージ率からサイズ、スピード、生存時間を計算
    size = SeedCore::Lerp(minSize, maxSize, chargeRate);//チャージされているほど大きい
    speed = SeedCore::Lerp(maxSpeed, minSpeed, chargeRate);//チャージされているほど遅い
    aliveTime = SeedCore::Lerp(minAliveTime, maxAliveTime, chargeRate);//チャージされているほど生存時間が長い

    //chargeRateによってサイズが分かるようにタグを追加
    if (chargeRate <= 0.33f)
    {
        GetActor().AddTag("BulletSmall");
    }
    else if (chargeRate <= 0.66f)
    {
        GetActor().AddTag("BulletMedium");
    }
    else
    {
        GetActor().AddTag("BulletBig");
    }

    //スケールをセット
    scale->x_ = size;
    scale->y_ = size;
    scale->z_ = size;

    //コストをセット
    this->cost = cost;
}

void BulletController::Stop()
{
    //停止処理
   
    //停止ステートにする
    state = State::STOP;

    //重力エリアであればRigidbodyをDynamicをする
    if (isGravityArea)
    {
        isDynamicWait = true;
    }
    
    //衝突判定ON
    rigidbody->isTrigger_ = false;
    rigidbody->Dispatch();

    //速度を0にする
    linearVelocity->x_ = 0.0f;
    linearVelocity->y_ = 0.0f;
    linearVelocity->z_ = 0.0f;
    //回転速度を0にする
    angularVelocity->x_ = 0.0f;
    angularVelocity->y_ = 0.0f;
    angularVelocity->z_ = 0.0f;

    //ストップタイマーを出現させる
    stopTimer = SeedCore::Prefab::Spawn(SeedCore::String("StopTimerText.prefab"));
    stopTimerText = stopTimer.GetComponent<SeedCore::Text>();
    //スクリーン座標変換&オフセットを適応
    //ただのテキスト表示なため回転を考慮しない
    stopTimerPosition = stopTimer.GetComponent<SeedCore::Position>();
    SeedCore::Vector3 worldPosition;
    worldPosition.x = position->x_ + stopTimerOffset.x;
    worldPosition.y = position->y_ + stopTimerOffset.y;
    worldPosition.z = position->z_ + stopTimerOffset.z;
    SeedCore::Vector2 screenPosition = SeedCore::ScreenSpace::WorldToScreen(worldPosition);
    stopTimerPosition->x_ = screenPosition.x;
    stopTimerPosition->y_ = screenPosition.y;
}

void BulletController::UpdateAliveTime(float elapsedTime)
{
    aliveTimer += elapsedTime;
    if (aliveTimer >= aliveTime)
    {
        //生存時間が終了したら削除
        Remove();
    }
}

void BulletController::UpdateStopTimer(float elapsedTime)
{
    //タイマーがなければ以下の処理を行わない
    if (!stopTimer)return;

    //タイマーの更新

    //残りの生存時間を停止時間とする
    float stopTime = aliveTime - aliveTimer;
    //小数点第一以下を切り捨て
    float stopTimeSca10 = stopTime * 10.0f;
    int stopTimeInt = static_cast<int>(stopTimeSca10);
    float stopTimeDisplay = static_cast<float>(stopTimeInt) / 10.0f;
    //テキスト反映
    std::string stopTimeStr = std::format("Stop {:.1f}", stopTimeDisplay);
    stopTimerText->text_ = SeedCore::String(stopTimeStr);

    //スクリーン座標変換&オフセットを適応
    SeedCore::Vector3 worldPosition;
    worldPosition.x = position->x_ + stopTimerOffset.x;
    worldPosition.y = position->y_ + stopTimerOffset.y;
    worldPosition.z = position->z_ + stopTimerOffset.z;
    SeedCore::Vector2 screenPosition = SeedCore::ScreenSpace::WorldToScreen(worldPosition);
    stopTimerPosition->x_ = screenPosition.x;
    stopTimerPosition->y_ = screenPosition.y;
}

void BulletController::Remove()
{
    playerController->SubCostGauge(cost);//自分が圧迫してた分のコストを戻す
    GetWorld().DestroyActor(GetActor());//削除

    if (state != State::STOP)return;//停止中じゃなければ以下の処理を行わない

    //ストップタイマーを削除
    GetWorld().DestroyActor(stopTimer);
    stopTimerText = nullptr;
    stopTimerPosition = nullptr;
}

void BulletController::Move(float elapsedTime)
{
    //移動・回転処理

    //停止中なら終了
    if (state != State::NORMAL || !position || !rotation || !rigidbody)
    {
        //常に回転0
        if (rigidbody && rigidbody->bodyType_ == SeedCore::Rigidbody::BodyType::Kinematic)
        {
            if(position)rigidbody->MoveTarget({ position->x_,position->y_,position->z_ }, SeedCore::Quaternion::Identity, elapsedTime);

            if (isDynamicWait)
            {
                dynamicWaitTimer -= elapsedTime;
                if (dynamicWaitTimer <= 0.0f)
                {
                    isDynamicWait = false;
                    rigidbody->bodyType_ = SeedCore::Rigidbody::BodyType::Dynamic;
                    rigidbody->freezePositionX_ = true;
                    rigidbody->freezePositionY_ = true;
                    rigidbody->freezePositionZ_ = true;
                    rigidbody->Dispatch();
                }
            }
        }

        return;
    }

    

    //moveDirectionの方向に移動
    SeedCore::Vector3 moveTarget = SeedCore::Transform::Vector(*position) + (moveDirection * speed * elapsedTime);

    //turnAxisを軸に回転
    SeedCore::Quaternion quaternion = SeedCore::Quaternion::CreateFromAxisAngle(turnAxis, turnSpeed * elapsedTime);
    quaternion = SeedCore::Transform::Quat(*rotation) * quaternion;

    //キネマティック剛体を動かす
    rigidbody->MoveTarget(moveTarget, quaternion, 0.02f);

    if ((SeedCore::Transform::Vector(*position) - startPosition).Length() >= playerController->GetBulletRemoveDistance())
    {
        //一定距離飛んだら消す
        Remove();
    }
}
